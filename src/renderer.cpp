#include "renderer.hpp"

#include <string>

#include "log.hpp"

namespace ar_overlay {

GLRenderer::GLRenderer(GstElement* glshader, const std::string& vertexSrc,
                       const std::string& fragmentSrc)
    : glshader_(glshader) {
  g_object_set(G_OBJECT(glshader_),
    "vertex", vertexSrc.c_str(),
    "fragment", fragmentSrc.c_str(),
    "update-shader", TRUE,
    nullptr);
}

void GLRenderer::setTextureSize(int width, int height) {
  if (width <= 0 || height <= 0) {
    logWarning("Invalid texture size: " + std::to_string(width) + "x" +
               std::to_string(height));
    return;
  }
  width_ = width;
  height_ = height;
}

void GLRenderer::updateAmplitudes(const std::vector<float>& dBValues) {
  if (!glshader_ || dBValues.empty()) return;
  if (width_ <= 0 || height_ <= 0) return;

  GstStructure* uniforms = gst_structure_new_empty("uniforms");

  for (int i = 0; i < kNumBands && i < static_cast<int>(dBValues.size()); ++i) {
    const float linear = normalizeDb(dBValues[i]);

    if (smoothedInitialized_) {
      smoothed_[i] = smoothingAlpha_ * linear +
                     (1.0f - smoothingAlpha_) * smoothed_[i];
    } else {
      smoothed_[i] = linear;
    }

    const std::string uniformName = "u_a[" + std::to_string(i) + "]";
    gst_structure_set(uniforms, uniformName.c_str(), G_TYPE_FLOAT,
                      static_cast<double>(smoothed_[i]), nullptr);
  }
  smoothedInitialized_ = true;

  gst_structure_set(uniforms,
    "u_texel_x", G_TYPE_FLOAT,
    static_cast<double>(1.0f / static_cast<float>(width_)),
    "u_texel_y", G_TYPE_FLOAT,
    static_cast<double>(1.0f / static_cast<float>(height_)),
    "u_blur",    G_TYPE_FLOAT, static_cast<double>(kDefaultBlurRadius),
    nullptr);

  g_object_set(G_OBJECT(glshader_), "uniforms", uniforms, nullptr);
  gst_structure_free(uniforms);
}

constexpr float GLRenderer::normalizeDb(float dB) noexcept {
  if (dB <= -60.0f) return 0.0f;
  if (dB >= 0.0f)   return 1.0f;
  return (dB + 60.0f) / 60.0f;
}

} // namespace ar_overlay