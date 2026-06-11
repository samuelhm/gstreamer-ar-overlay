/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   renderer.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:04:08 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:09 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <gst/gst.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace ar_overlay {

class GLRenderer {
public:
  static constexpr int kNumBands = 16;

  GLRenderer() = default;
  ~GLRenderer() = default;

  GLRenderer(const GLRenderer&) = delete;
  GLRenderer& operator=(const GLRenderer&) = delete;
  GLRenderer(GLRenderer&&) noexcept = default;
  GLRenderer& operator=(GLRenderer&&) noexcept = default;

  void configure(GstElement* glshader, const std::string& vertexSrc,
                 const std::string& fragmentSrc);
  void setTextureSize(int width, int height);
  void setSmoothing(float alpha) { smoothingAlpha_ = alpha; }
  void updateAmplitudes(const std::vector<float>& dBValues);

private:
  static float dBtoLinear(float dB);

  GstElement* glshader_ = nullptr;
  float smoothingAlpha_ = 0.3f;
  int width_ = 0;
  int height_ = 0;
  bool configured_ = false;
};

} // namespace ar_overlay
