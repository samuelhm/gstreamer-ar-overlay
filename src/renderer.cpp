#include "renderer.hpp"
#include "log.hpp"

#include <cmath>

namespace ar_overlay {

GLRenderer::GLRenderer(GstElement* glshader, const std::string& vertexSrc,
                       const std::string& fragmentSrc)
    : glshader_(glshader) {
  g_object_set(G_OBJECT(glshader_),
    "vertex", vertexSrc.c_str(),
    "fragment", fragmentSrc.c_str(),
    nullptr);
}

void GLRenderer::setTextureSize(int width, int height) {
  if (width <= 0 || height <= 0) {
    logError("Invalid texture size: " + std::to_string(width) + "x" +
             std::to_string(height));
    return;
  }
  width_ = width;
  height_ = height;
}

void GLRenderer::updateAmplitudes(const std::vector<float>& dBValues) {
  if (!glshader_ || dBValues.empty()) return;
  if (width_ <= 0 || height_ <= 0) return;

  GstStructure* s = gst_structure_new_empty("uniforms");

  constexpr std::array<std::string_view, kNumBands> kNames = {
    "u_a0",  "u_a1",  "u_a2",  "u_a3",  "u_a4",  "u_a5",  "u_a6",  "u_a7",
    "u_a8",  "u_a9",  "u_a10", "u_a11", "u_a12", "u_a13", "u_a14", "u_a15"
  };

  for (int i = 0; i < kNumBands && i < static_cast<int>(dBValues.size()); ++i) {
    gst_structure_set(s, kNames[i].data(), G_TYPE_FLOAT,
                      static_cast<double>(dBtoLinear(dBValues[i])), nullptr);
  }

  gst_structure_set(s,
    "u_texel_x", G_TYPE_FLOAT,
    static_cast<double>(1.0f / static_cast<float>(width_)),
    "u_texel_y", G_TYPE_FLOAT,
    static_cast<double>(1.0f / static_cast<float>(height_)),
    "u_blur",    G_TYPE_FLOAT, static_cast<double>(kDefaultBlurRadius),
    nullptr);

  g_object_set(G_OBJECT(glshader_), "uniforms", s, nullptr);
  g_object_set(G_OBJECT(glshader_), "update-shader", TRUE, nullptr);
  gst_structure_free(s);
}

constexpr float GLRenderer::dBtoLinear(float dB) noexcept {
  if (dB <= -60.0f) return 0.0f;
  if (dB >= 0.0f)   return 1.0f;
  return (dB + 60.0f) / 60.0f;
}

} // namespace ar_overlay
