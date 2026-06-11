#include "spectrum.hpp"

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

  const GstStructure* structure = gst_message_get_structure(msg);
  if (!gst_structure_has_name(structure, "spectrum")) return false;

  const GValue* magnitudeList = gst_structure_get_value(structure, "magnitude");
  if (!magnitudeList) return false;

  const guint count = gst_value_list_get_size(magnitudeList);
  for (guint i = 0; i < count && i < bands_; ++i) {
    const GValue* item = gst_value_list_get_value(magnitudeList, i);
    magnitudes_[i] = G_VALUE_HOLDS_FLOAT(item)
                         ? g_value_get_float(item)
                         : 0.0f;
  }

  return true;
}

} // namespace ar_overlay