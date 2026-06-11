/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipeline.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 10:08:08 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/09 10:08:09 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <gst/gst.h>
#include <memory>
#include <optional>
#include <string_view>

#include "spectrum.hpp"

namespace ar_overlay {

struct GstElementDeleter {
  void operator()(GstElement* e) const noexcept {
    if (e) {
      gst_element_set_state(e, GST_STATE_NULL);
      gst_object_unref(e);
    }
  }
};

struct GMainLoopDeleter {
  void operator()(GMainLoop* loop) const noexcept {
    if (loop) {
      g_main_loop_unref(loop);
    }
  }
};

using GstElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;
using GMainLoopPtr = std::unique_ptr<GMainLoop, GMainLoopDeleter>;

class Pipeline {
public:
  explicit Pipeline(std::string_view filePath);
  ~Pipeline() = default;

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&&) noexcept = default;
  Pipeline& operator=(Pipeline&&) noexcept = default;

  bool run();

private:
  static void onPadAdded(GstElement* src, GstPad* newPad, gpointer data);
  static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);

  void handleMessage(GstMessage* msg);

  GMainLoopPtr mainLoop_;
  GstElementPtr pipeline_;
  std::optional<SpectrumAnalyzer> spectrumAnalyzer_;
  int spectrumFrameCount_ = 0;
  bool hasError_ = false;
};

} // namespace ar_overlay
