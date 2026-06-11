#pragma once

#include <gst/gst.h>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "spectrum.hpp"
#include "renderer.hpp"

// Forward declaration — avoids pulling in the full GTK4 header.
typedef struct _GdkPaintable GdkPaintable;

namespace ar_overlay {

struct GstElementDeleter {
  void operator()(GstElement* e) const noexcept {
    if (e) {
      gst_element_set_state(e, GST_STATE_NULL);
      gst_object_unref(e);
    }
  }
};

struct GstCapsDeleter {
  void operator()(GstCaps* c) const noexcept {
    if (c) gst_caps_unref(c);
  }
};

using GstElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;
using GstCapsPtr = std::unique_ptr<GstCaps, GstCapsDeleter>;

class Pipeline {
public:
  explicit Pipeline(std::string_view filePath);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&&) noexcept = default;
  Pipeline& operator=(Pipeline&&) noexcept = default;

  bool start();

  // Non-owning. Lifetime managed by Pipeline (g_clear_object in destructor).
  GdkPaintable* paintable() const { return paintable_; }

  void setQuitCallback(std::function<void()> cb) { quitCb_ = std::move(cb); }

private:
  static void onPadAdded(GstElement* src, GstPad* newPad, gpointer data);
  static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);
  static gboolean onSignal(gpointer data);

  void handleMessage(GstMessage* msg);
  void handleAudioPad(GstPad* newPad, const GstCaps* caps);
  void handleVideoPad(GstPad* newPad, const GstCaps* caps);

  GstElementPtr pipeline_;
  GdkPaintable* paintable_ = nullptr;
  std::optional<SpectrumAnalyzer> spectrumAnalyzer_;
  std::optional<GLRenderer> renderer_;
  std::function<void()> quitCb_;
  guint sigintSource_ = 0;
  guint sigtermSource_ = 0;

  std::string vertexSrc_;
  std::string fragmentSrc_;
};

} // namespace ar_overlay
