#pragma once

#include <vector>

#include <gst/gst.h>

namespace ar_overlay {

class SpectrumAnalyzer {
public:
  static constexpr guint kDefaultBands = 16;

  SpectrumAnalyzer() = default;

  SpectrumAnalyzer(const SpectrumAnalyzer&) = delete;
  SpectrumAnalyzer& operator=(const SpectrumAnalyzer&) = delete;
  SpectrumAnalyzer(SpectrumAnalyzer&&) noexcept = default;
  SpectrumAnalyzer& operator=(SpectrumAnalyzer&&) noexcept = default;

  void attach(GstElement* spectrum, guint bands = kDefaultBands);

  bool processMessage(GstMessage* msg);

  const std::vector<float>& magnitudes() const noexcept { return magnitudes_; }
  guint bands() const noexcept { return bands_; }

private:
  std::vector<float> magnitudes_;
  guint bands_ = 0;
  GstElement* spectrum_ = nullptr; // Non-owning. Must not outlive the element.
};

} // namespace ar_overlay
