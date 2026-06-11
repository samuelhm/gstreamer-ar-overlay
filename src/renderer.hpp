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
  static constexpr float kDefaultBlurRadius = 15.0f;
  static constexpr float kDefaultSmoothingAlpha = 0.3f;

  GLRenderer(GstElement* glshader, const std::string& vertexSrc,
             const std::string& fragmentSrc);

  GLRenderer(const GLRenderer&) = delete;
  GLRenderer& operator=(const GLRenderer&) = delete;
  GLRenderer(GLRenderer&&) noexcept = default;
  GLRenderer& operator=(GLRenderer&&) noexcept = default;

  void setTextureSize(int width, int height);
  void setSmoothing(float alpha) { smoothingAlpha_ = alpha; }
  void updateAmplitudes(const std::vector<float>& dBValues);

private:
  static constexpr float dBtoLinear(float dB) noexcept;

  GstElement* glshader_ = nullptr;
  float smoothingAlpha_ = kDefaultSmoothingAlpha;
  int width_ = 0;
  int height_ = 0;
};

} // namespace ar_overlay
