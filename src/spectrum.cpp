#include "spectrum.hpp"

#include <iostream>

namespace ar_overlay {

void SpectrumAnalyzer::attach(GstElement* spectrum, guint bands) {
  spectrum_ = spectrum;
  bands_ = bands;
  magnitudes_.resize(bands_);

  g_object_set(G_OBJECT(spectrum),
    "bands", bands_,
    "post-messages", TRUE,
    "message-phase", FALSE,
    "interval", G_GUINT64_CONSTANT(30000000),
    nullptr);
}

bool SpectrumAnalyzer::processMessage(GstMessage* msg) {
  if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ELEMENT) return false;
  if (GST_MESSAGE_SRC(msg) != GST_OBJECT_CAST(spectrum_)) return false;

  const GstStructure* s = gst_message_get_structure(msg);
  if (!gst_structure_has_name(s, "spectrum")) return false;

  const GValue* val = gst_structure_get_value(s, "magnitude");
  if (!val) return false;

  const guint count = gst_value_list_get_size(val);
  for (guint i = 0; i < count && i < bands_; ++i) {
    const GValue* item = gst_value_list_get_value(val, i);
    magnitudes_[i] = static_cast<float>(g_value_get_float(item));
  }

  return true;
}

} // namespace ar_overlay
