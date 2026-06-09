#include "pipeline.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace ar_overlay {

Pipeline::Pipeline(std::string_view filePath)
  : filePath_(filePath) {

  pipeline_ = gst_pipeline_new("main_pipeline");
  GstElement* decodebin = gst_element_factory_make("uridecodebin", "decoder");

  if (!pipeline_ || !decodebin) {
    throw std::runtime_error("Failed to create GStreamer elements");
  }

  g_object_set(G_OBJECT(decodebin), "uri", filePath_.c_str(), nullptr);

  g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), pipeline_);

  gst_bin_add(GST_BIN(pipeline_), decodebin);

  mainLoop_ = g_main_loop_new(nullptr, FALSE);

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
  gst_bus_add_watch(bus, onBusMessage, this);
  gst_object_unref(bus);
}

Pipeline::~Pipeline() {
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
  }
  if (mainLoop_) {
    g_main_loop_unref(mainLoop_);
  }
}

Pipeline::Pipeline(Pipeline&& other) noexcept
  : pipeline_(std::exchange(other.pipeline_, nullptr))
  , mainLoop_(std::exchange(other.mainLoop_, nullptr))
  , filePath_(std::move(other.filePath_)) {}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
  if (this != &other) {
    if (pipeline_) {
      gst_element_set_state(pipeline_, GST_STATE_NULL);
      gst_object_unref(pipeline_);
    }
    if (mainLoop_) {
      g_main_loop_unref(mainLoop_);
    }
    pipeline_ = std::exchange(other.pipeline_, nullptr);
    mainLoop_ = std::exchange(other.mainLoop_, nullptr);
    filePath_ = std::move(other.filePath_);
  }
  return *this;
}

void Pipeline::run() {
  gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  g_main_loop_run(mainLoop_);
}

void Pipeline::onPadAdded(GstElement* /*src*/, GstPad* newPad, gpointer data) {
  auto* pipeline = static_cast<GstElement*>(data);

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

  GstElement* sink = nullptr;

  if (g_str_has_prefix(name, "audio/")) {
    sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  } else if (g_str_has_prefix(name, "video/")) {
    sink = gst_element_factory_make("autovideosink", "video_sink");
  }

  gst_caps_unref(caps);

  if (!sink) {
    return;
  }

  gst_bin_add(GST_BIN(pipeline), sink);
  gst_element_sync_state_with_parent(sink);

  GstPad* sinkPad = gst_element_get_static_pad(sink, "sink");
  GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
  gst_object_unref(sinkPad);

  if (ret != GST_PAD_LINK_OK) {
    std::cerr << "Failed to link pad: " << name << "\n";
  }
}

gboolean Pipeline::onBusMessage(GstBus* /*bus*/, GstMessage* msg, gpointer data) {
  auto* self = static_cast<Pipeline*>(data);
  self->handleMessage(msg);
  return TRUE;
}

void Pipeline::handleMessage(GstMessage* msg) {
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gchar* debug = nullptr;
      gst_message_parse_error(msg, &err, &debug);
      std::cerr << "Error: " << err->message << "\n";
      g_error_free(err);
      g_free(debug);
      g_main_loop_quit(mainLoop_);
      break;
    }
    case GST_MESSAGE_EOS:
      std::cout << "End of stream\n";
      g_main_loop_quit(mainLoop_);
      break;
    default:
      break;
  }
}

} // namespace ar_overlay
