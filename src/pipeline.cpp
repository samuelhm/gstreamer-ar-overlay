#include <stdexcept>

#include <glib-unix.h>

#include "log.hpp"
#include "pipeline.hpp"
#include "shader_loader.hpp"

namespace ar_overlay {

Pipeline::Pipeline(std::string_view filePath) {
  vertexSrc_ = loadShaderFile("shaders/default.vert");
  fragmentSrc_ = loadShaderFile("shaders/eq_columns.frag");

  pipeline_.reset(gst_pipeline_new("main_pipeline"));
  GstElement* decodebin = gst_element_factory_make("uridecodebin", "decoder");

  GstElement* glsinkbin = gst_element_factory_make("glsinkbin", "glsinkbin");
  GstElement* gtk4sink = gst_element_factory_make("gtk4paintablesink", "gtk4paintablesink");

  if (!pipeline_ || !decodebin || !glsinkbin || !gtk4sink) {
    throw std::runtime_error("Failed to create GStreamer elements");
  }

  g_object_set(glsinkbin, "sink", gtk4sink, nullptr);
  g_object_get(gtk4sink, "paintable", &paintable_, nullptr);

  gst_bin_add(GST_BIN(pipeline_.get()), glsinkbin);

  const std::string path(filePath);
  gchar* uri = gst_filename_to_uri(path.c_str(), nullptr);
  if (!uri) {
    throw std::runtime_error("Failed to convert file path to URI");
  }
  g_object_set(G_OBJECT(decodebin), "uri", uri, nullptr);

  g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), this);

  gst_bin_add(GST_BIN(pipeline_.get()), decodebin);

  g_free(uri);

  sigintSource_ = g_unix_signal_add(SIGINT, onSignal, this);
  sigtermSource_ = g_unix_signal_add(SIGTERM, onSignal, this);

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_.get()));
  gst_bus_add_watch(bus, onBusMessage, this);
  gst_object_unref(bus);
}

Pipeline::~Pipeline() {
  if (sigintSource_ > 0) g_source_remove(sigintSource_);
  if (sigtermSource_ > 0) g_source_remove(sigtermSource_);
  g_clear_object(&paintable_);
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : pipeline_(std::move(other.pipeline_))
    , paintable_(other.paintable_)
    , spectrumAnalyzer_(std::move(other.spectrumAnalyzer_))
    , renderer_(std::move(other.renderer_))
    , quitCb_(std::move(other.quitCb_))
    , sigintSource_(other.sigintSource_)
    , sigtermSource_(other.sigtermSource_)
    , vertexSrc_(std::move(other.vertexSrc_))
    , fragmentSrc_(std::move(other.fragmentSrc_))
{
  other.paintable_ = nullptr;
  other.sigintSource_ = 0;
  other.sigtermSource_ = 0;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
  if (this != &other) {
    if (sigintSource_ > 0) g_source_remove(sigintSource_);
    if (sigtermSource_ > 0) g_source_remove(sigtermSource_);
    g_clear_object(&paintable_);

    pipeline_ = std::move(other.pipeline_);
    paintable_ = other.paintable_;
    other.paintable_ = nullptr;
    spectrumAnalyzer_ = std::move(other.spectrumAnalyzer_);
    renderer_ = std::move(other.renderer_);
    quitCb_ = std::move(other.quitCb_);
    sigintSource_ = other.sigintSource_;
    other.sigintSource_ = 0;
    sigtermSource_ = other.sigtermSource_;
    other.sigtermSource_ = 0;
    vertexSrc_ = std::move(other.vertexSrc_);
    fragmentSrc_ = std::move(other.fragmentSrc_);
  }
  return *this;
}

bool Pipeline::start() {
  GstStateChangeReturn stateResult = gst_element_set_state(pipeline_.get(), GST_STATE_PLAYING);
  if (stateResult == GST_STATE_CHANGE_FAILURE) {
    logError("Failed to start pipeline");
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Static callbacks
// ---------------------------------------------------------------------------

void Pipeline::onPadAdded(GstElement* /*src*/, GstPad* newPad, gpointer data) {
  if (!data) return;
  auto* self = static_cast<Pipeline*>(data);

  GstCapsPtr caps;
  {
    GstCaps* rawCaps = gst_pad_get_current_caps(newPad);
    if (!rawCaps) rawCaps = gst_pad_query_caps(newPad, nullptr);
    caps.reset(rawCaps);
  }

  if (!caps) {
    logError("Failed to query pad caps");
    return;
  }

  GstStructure* structure = gst_caps_get_structure(caps.get(), 0);
  const gchar* name = gst_structure_get_name(structure);

  if (g_str_has_prefix(name, "audio/")) {
    self->handleAudioPad(newPad, caps.get());
  } else if (g_str_has_prefix(name, "video/")) {
    self->handleVideoPad(newPad, caps.get());
  }
}

gboolean Pipeline::onBusMessage(GstBus* /*bus*/, GstMessage* msg, gpointer data) {
  if (!data) return FALSE;
  auto* self = static_cast<Pipeline*>(data);
  self->handleMessage(msg);
  return TRUE;
}

gboolean Pipeline::onSignal(gpointer data) {
  if (!data) return G_SOURCE_REMOVE;
  auto* self = static_cast<Pipeline*>(data);
  if (self->quitCb_) self->quitCb_();
  return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------
// Pad handlers
// ---------------------------------------------------------------------------

void Pipeline::handleAudioPad(GstPad* newPad, const GstCaps* /*caps*/) {
  GstElement* pipeline = pipeline_.get();

  if (!spectrumAnalyzer_.has_value()) {
    GstElement* audioconvert = gst_element_factory_make("audioconvert", nullptr);
    GstElement* spectrum = gst_element_factory_make("spectrum", nullptr);
    GstElement* audiosink = gst_element_factory_make("autoaudiosink", nullptr);

    if (!audioconvert || !spectrum || !audiosink) {
      logError("Failed to create audio elements");
      return;
    }

    gst_bin_add_many(GST_BIN(pipeline), audioconvert, spectrum, audiosink, nullptr);
    gst_element_sync_state_with_parent(audioconvert);
    gst_element_sync_state_with_parent(spectrum);
    gst_element_sync_state_with_parent(audiosink);

    gst_element_link_many(audioconvert, spectrum, audiosink, nullptr);

    GstPad* convSinkPad = gst_element_get_static_pad(audioconvert, "sink");
    GstPadLinkReturn linkResult = gst_pad_link(newPad, convSinkPad);
    gst_object_unref(convSinkPad);

    if (linkResult != GST_PAD_LINK_OK) {
      logWarning("Failed to link audio pad");
    }

    spectrumAnalyzer_.emplace();
    spectrumAnalyzer_->attach(spectrum);
  } else {
    GstElement* audiosink = gst_element_factory_make("autoaudiosink", nullptr);

    if (!audiosink) return;

    gst_bin_add(GST_BIN(pipeline), audiosink);
    gst_element_sync_state_with_parent(audiosink);

    GstPad* additionalSinkPad = gst_element_get_static_pad(audiosink, "sink");
    GstPadLinkReturn linkResult = gst_pad_link(newPad, additionalSinkPad);
    gst_object_unref(additionalSinkPad);

    if (linkResult != GST_PAD_LINK_OK) {
      logWarning("Failed to link additional audio pad");
    }
  }
}

void Pipeline::handleVideoPad(GstPad* newPad, const GstCaps* caps) {
  GstElement* pipeline = pipeline_.get();

  GstElement* videoconvert = gst_element_factory_make("videoconvert", nullptr);
  GstElement* glupload = gst_element_factory_make("glupload", nullptr);
  GstElement* glshader = gst_element_factory_make("glshader", nullptr);

  if (!videoconvert || !glupload || !glshader) {
    logError("Failed to create GL video elements");
    return;
  }

  GstElement* glsinkbin = gst_bin_get_by_name(GST_BIN(pipeline), "glsinkbin");
  if (!glsinkbin) {
    logError("glsinkbin not found in pipeline");
    return;
  }

  GstStructure* structure = gst_caps_get_structure(caps, 0);
  gint width = 0, height = 0;
  gst_structure_get_int(structure, "width", &width);
  gst_structure_get_int(structure, "height", &height);

  renderer_.emplace(glshader, vertexSrc_, fragmentSrc_);

  gst_bin_add_many(GST_BIN(pipeline), videoconvert,
                   glupload, glshader, nullptr);
  gst_element_sync_state_with_parent(videoconvert);
  gst_element_sync_state_with_parent(glupload);
  gst_element_sync_state_with_parent(glshader);

  gst_element_link_many(videoconvert,
                        glupload, glshader, glsinkbin, nullptr);

  gst_object_unref(glsinkbin);

    GstPad* videoSinkPad = gst_element_get_static_pad(videoconvert, "sink");
    GstPadLinkReturn linkResult = gst_pad_link(newPad, videoSinkPad);
    gst_object_unref(videoSinkPad);

    if (linkResult != GST_PAD_LINK_OK) {
      logWarning("Failed to link video GL pad");
    }

  if (width > 0 && height > 0) {
    renderer_->setTextureSize(width, height);
  }
}

// ---------------------------------------------------------------------------
// Bus message handling
// ---------------------------------------------------------------------------

void Pipeline::handleMessage(GstMessage* msg) {
  if (spectrumAnalyzer_ && spectrumAnalyzer_->processMessage(msg)) {
    if (renderer_) {
      renderer_->updateAmplitudes(spectrumAnalyzer_->magnitudes());
    }
    return;
  }

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gchar* debug = nullptr;
      gst_message_parse_error(msg, &err, &debug);
      logError(std::string("GStreamer: ") + err->message);
      if (debug) {
        logError(std::string("Debug: ") + debug);
      }
      g_error_free(err);
      g_free(debug);
      if (quitCb_) quitCb_();
      break;
    }
    case GST_MESSAGE_EOS:
      if (quitCb_) quitCb_();
      break;
    case GST_MESSAGE_ELEMENT: {
      const GstStructure* structure = gst_message_get_structure(msg);
      if (gst_structure_has_name(structure, "compat")) {
        const gchar* shaderError = gst_structure_get_string(structure, "error");
        if (shaderError) {
          logWarning(std::string("GL shader: ") + shaderError);
        }
      }
      break;
    }
    default:
      break;
  }
}

} // namespace ar_overlay
