/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipeline.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:04:12 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:13 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <gst/gst.h>
#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "spectrum.hpp"
#include "renderer.hpp"

namespace ar_overlay {

struct GstElementDeleter {
  void operator()(GstElement* e) const noexcept {
    if (e) {
      gst_element_set_state(e, GST_STATE_NULL);
      gst_object_unref(e);
    }
  }
};

using GstElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;

class Pipeline {
public:
  explicit Pipeline(std::string_view filePath);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&&) noexcept = default;
  Pipeline& operator=(Pipeline&&) noexcept = default;

  bool start();
  GdkPaintable* paintable() const { return paintable_; }
  void setQuitCallback(std::function<void()> cb) { quitCb_ = std::move(cb); }

private:
  static void onPadAdded(GstElement* src, GstPad* newPad, gpointer data);
  static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);
  static gboolean onSignal(gpointer data);

  void handleMessage(GstMessage* msg);

  GstElementPtr pipeline_;
  GdkPaintable* paintable_ = nullptr;
  std::optional<SpectrumAnalyzer> spectrumAnalyzer_;
  std::optional<GLRenderer> renderer_;
  std::function<void()> quitCb_;
  guint sigintSource_ = 0;
  guint sigtermSource_ = 0;
};

} // namespace ar_overlay
