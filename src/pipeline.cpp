/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipeline.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 10:08:18 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/09 10:08:19 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pipeline.hpp"

#include <iostream>
#include <stdexcept>

namespace ar_overlay {

Pipeline::Pipeline(std::string_view filePath) {
  pipeline_.reset(gst_pipeline_new("main_pipeline"));
  GstElement* decodebin = gst_element_factory_make("uridecodebin", "decoder");

  if (!pipeline_ || !decodebin) {
    throw std::runtime_error("Failed to create GStreamer elements");
  }

  const std::string uri(filePath);
  g_object_set(G_OBJECT(decodebin), "uri", uri.c_str(), nullptr);

  g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), pipeline_.get());

  gst_bin_add(GST_BIN(pipeline_.get()), decodebin);

  mainLoop_.reset(g_main_loop_new(nullptr, FALSE));

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
