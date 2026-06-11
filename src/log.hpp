#pragma once

#include <gst/gst.h>

#include <string>

namespace ar_overlay {

inline void logError(const std::string& msg) {
  g_printerr("ERROR: %s\n", msg.c_str());
}

} // namespace ar_overlay
