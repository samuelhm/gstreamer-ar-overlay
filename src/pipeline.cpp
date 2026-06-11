#include "pipeline.hpp"
#include "shader_loader.hpp"

#include <glib-unix.h>

#include <iostream>
#include <stdexcept>

namespace ar_overlay {

Pipeline::Pipeline(std::string_view filePath) {
  pipeline_.reset(gst_pipeline_new("main_pipeline"));
  GstElement* decodebin = gst_element_factory_make("uridecodebin", "decoder");

  if (!pipeline_ || !decodebin) {
    throw std::runtime_error("Failed to create GStreamer elements");
  }

  const std::string path(filePath);
  gchar* uri = gst_filename_to_uri(path.c_str(), nullptr);
  if (!uri) {
    throw std::runtime_error("Failed to convert file path to URI");
  }
  g_object_set(G_OBJECT(decodebin), "uri", uri, nullptr);

  g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), this);

  gst_bin_add(GST_BIN(pipeline_.get()), decodebin);

  g_free(uri);

  mainLoop_.reset(g_main_loop_new(nullptr, FALSE));

  g_unix_signal_add(SIGINT, onSignal, this);
  g_unix_signal_add(SIGTERM, onSignal, this);

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_.get()));
  gst_bus_add_watch(bus, onBusMessage, this);
  gst_object_unref(bus);
}

bool Pipeline::run() {
  GstStateChangeReturn ret = gst_element_set_state(pipeline_.get(), GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    std::cerr << "Failed to start pipeline\n";
    return false;
  }
  g_main_loop_run(mainLoop_.get());
  return !hasError_;
}

void Pipeline::onPadAdded(GstElement* /*src*/, GstPad* newPad, gpointer data) {
  auto* self = static_cast<Pipeline*>(data);
  GstElement* pipeline = self->pipeline_.get();

  GstCaps* caps = gst_pad_get_current_caps(newPad);
  if (!caps) {
    caps = gst_pad_query_caps(newPad, nullptr);
  }

  if (!caps) {
    std::cerr << "Failed to query pad caps\n";
    return;
  }

  GstStructure* structure = gst_caps_get_structure(caps, 0);
  const gchar* name = gst_structure_get_name(structure);

  if (g_str_has_prefix(name, "audio/")) {
    const bool firstAudio = !self->spectrumAnalyzer_.has_value();

    if (firstAudio) {
      GstElement* audioconvert = gst_element_factory_make("audioconvert", nullptr);
      GstElement* spectrum = gst_element_factory_make("spectrum", nullptr);
      GstElement* audiosink = gst_element_factory_make("autoaudiosink", nullptr);

      if (!audioconvert || !spectrum || !audiosink) {
        std::cerr << "Failed to create audio elements\n";
        gst_caps_unref(caps);
        return;
      }

      gst_bin_add_many(GST_BIN(pipeline), audioconvert, spectrum, audiosink, nullptr);
      gst_element_sync_state_with_parent(audioconvert);
      gst_element_sync_state_with_parent(spectrum);
      gst_element_sync_state_with_parent(audiosink);

      gst_element_link_many(audioconvert, spectrum, audiosink, nullptr);

      GstPad* convSinkPad = gst_element_get_static_pad(audioconvert, "sink");
      GstPadLinkReturn ret = gst_pad_link(newPad, convSinkPad);
      gst_object_unref(convSinkPad);

      if (ret != GST_PAD_LINK_OK) {
        std::cerr << "Failed to link audio pad\n";
      }

      self->spectrumAnalyzer_.emplace();
      self->spectrumAnalyzer_->attach(spectrum);
    } else {
      GstElement* audiosink = gst_element_factory_make("autoaudiosink", nullptr);

      if (!audiosink) {
        gst_caps_unref(caps);
        return;
      }

      gst_bin_add(GST_BIN(pipeline), audiosink);
      gst_element_sync_state_with_parent(audiosink);

      GstPad* sinkPad = gst_element_get_static_pad(audiosink, "sink");
      GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
      gst_object_unref(sinkPad);

      if (ret != GST_PAD_LINK_OK) {
        std::cerr << "Failed to link additional audio pad\n";
      }
    }

  } else if (g_str_has_prefix(name, "video/")) {
    GstElement* videoconvert = gst_element_factory_make("videoconvert", nullptr);
    GstElement* videoscale = gst_element_factory_make("videoscale", nullptr);
    GstElement* capsfilter = gst_element_factory_make("capsfilter", nullptr);
    GstElement* glupload = gst_element_factory_make("glupload", nullptr);
    GstElement* glshader = gst_element_factory_make("glshader", nullptr);
    GstElement* glconvert = gst_element_factory_make("glcolorconvert", nullptr);
    GstElement* videosink = gst_element_factory_make("glimagesinkelement", nullptr);

    if (!videoconvert || !videoscale || !capsfilter ||
        !glupload || !glshader || !glconvert || !videosink) {
      std::cerr << "Failed to create GL video elements\n";
      gst_caps_unref(caps);
      return;
    }

    gint width = 0, height = 0;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    if (width > 0 && height > 0) {
      GstCaps* scaleCaps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width * 2,
        "height", G_TYPE_INT, height * 2,
        nullptr);
      g_object_set(G_OBJECT(capsfilter), "caps", scaleCaps, nullptr);
      gst_caps_unref(scaleCaps);
    }

    ShaderLoader loader;
    self->renderer_.emplace();
    self->renderer_->configure(glshader,
                               loader.loadVertex("shaders/default.vert"),
                               loader.loadFragment("shaders/eq_columns.frag"));

    gst_bin_add_many(GST_BIN(pipeline), videoconvert, videoscale, capsfilter,
                     glupload, glshader, glconvert, videosink, nullptr);
    gst_element_sync_state_with_parent(videoconvert);
    gst_element_sync_state_with_parent(videoscale);
    gst_element_sync_state_with_parent(capsfilter);
    gst_element_sync_state_with_parent(glupload);
    gst_element_sync_state_with_parent(glshader);
    gst_element_sync_state_with_parent(glconvert);
    gst_element_sync_state_with_parent(videosink);

    gst_element_link_many(videoconvert, videoscale, capsfilter,
                          glupload, glshader, glconvert, videosink, nullptr);

    GstPad* convSinkPad = gst_element_get_static_pad(videoconvert, "sink");
    GstPadLinkReturn ret = gst_pad_link(newPad, convSinkPad);
    gst_object_unref(convSinkPad);

    if (ret != GST_PAD_LINK_OK) {
      std::cerr << "Failed to link video GL pad\n";
    }

    if (width > 0 && height > 0) {
      self->renderer_->setTextureSize(width * 2, height * 2);
    }

    gst_caps_unref(caps);
    return;
  }

  gst_caps_unref(caps);
}

gboolean Pipeline::onBusMessage(GstBus* /*bus*/, GstMessage* msg, gpointer data) {
  auto* self = static_cast<Pipeline*>(data);
  self->handleMessage(msg);
  return TRUE;
}

gboolean Pipeline::onSignal(gpointer data) {
  auto* self = static_cast<Pipeline*>(data);
  std::cout << "\nShutting down" << std::endl;
  self->interrupted_ = true;
  g_main_loop_quit(self->mainLoop_.get());
  return G_SOURCE_REMOVE;
}

void Pipeline::handleMessage(GstMessage* msg) {
  if (spectrumAnalyzer_ && spectrumAnalyzer_->processMessage(msg)) {
    if (renderer_) {
      renderer_->updateAmplitudes(spectrumAnalyzer_->magnitudes());
    }

    ++spectrumFrameCount_;
    if (spectrumFrameCount_ % 30 == 0) {
      const auto& mags = spectrumAnalyzer_->magnitudes();
      std::cout << "Magnitudes[" << spectrumFrameCount_ << "]: [";
      for (std::size_t i = 0; i < mags.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << mags[i];
      }
      std::cout << "]" << std::endl;
    }
    return;
  }

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gchar* debug = nullptr;
      gst_message_parse_error(msg, &err, &debug);
      std::cerr << "Error: " << err->message << std::endl;
      g_error_free(err);
      g_free(debug);
      hasError_ = true;
      g_main_loop_quit(mainLoop_.get());
      break;
    }
    case GST_MESSAGE_EOS:
      std::cout << "End of stream\n";
      g_main_loop_quit(mainLoop_.get());
      break;
    default:
      break;
  }
}

} // namespace ar_overlay
