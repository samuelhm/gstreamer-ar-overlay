#pragma once

#include <gst/gst.h>
#include <string>

namespace ar_overlay {

class Pipeline {
public:
  explicit Pipeline(std::string_view filePath);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&&) noexcept;
  Pipeline& operator=(Pipeline&&) noexcept;

  void run();

private:
  static void onPadAdded(GstElement* src, GstPad* newPad, gpointer data);
  static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);

  void handleMessage(GstMessage* msg);

  GstElement* pipeline_ = nullptr;
  GMainLoop* mainLoop_ = nullptr;
  std::string filePath_;
};

} // namespace ar_overlay
