/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   spectrum.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:03:59 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:00 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <gst/gst.h>

#include <cstdint>
#include <vector>

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
  GstElement* spectrum_ = nullptr;
};

} // namespace ar_overlay
