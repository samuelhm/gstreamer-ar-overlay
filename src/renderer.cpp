#include "renderer.hpp"

#include <cmath>
#include <cstdio>

namespace ar_overlay {

void GLRenderer::configure(GstElement* glshader, const std::string& vertexSrc,
                           const std::string& fragmentSrc) {
  glshader_ = glshader;

  g_object_set(G_OBJECT(glshader_),
    "vertex", vertexSrc.c_str(),
    "fragment", fragmentSrc.c_str(),
    nullptr);

  GstPad* srcPad = gst_element_get_static_pad(glshader_, "src");
  if (srcPad) {
    gst_pad_add_probe(srcPad, GST_PAD_PROBE_TYPE_BUFFER,
                      onBufferProbe, this, nullptr);
    gst_object_unref(srcPad);
  }

  configured_ = true;
}

void GLRenderer::setTextureSize(int width, int height) {
  width_ = width;
  height_ = height;
}

GstPadProbeReturn GLRenderer::onBufferProbe(GstPad* /*pad*/,
                                            GstPadProbeInfo* /*info*/,
                                            gpointer data) {
  auto* self = static_cast<GLRenderer*>(data);

  GstGLShader* shader = nullptr;
  g_object_get(G_OBJECT(self->glshader_), "shader", &shader, nullptr);
  if (!shader) return GST_PAD_PROBE_OK;

  self->applyUniforms(shader);
  gst_object_unref(shader);

  return GST_PAD_PROBE_OK;
}

void GLRenderer::applyUniforms(GstGLShader* shader) {
  if (!textureUniformsSet_) {
    applyTextureUniforms(shader);
  }

  std::vector<float> amps;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pendingAmplitudes_.empty()) return;
    amps = pendingAmplitudes_;
  }

  gst_gl_shader_set_uniform_1fv(shader, "u_amplitudes",
                                amps.size(), amps.data());

  for (std::size_t i = 0; i < amps.size(); ++i) {
    char name[64];
    std::snprintf(name, sizeof(name), "u_amplitudes[%u]",
                  static_cast<unsigned>(i));
    gst_gl_shader_set_uniform_1f(shader, name, amps[i]);
  }
}

void GLRenderer::applyTextureUniforms(GstGLShader* shader) {
  if (width_ <= 0 || height_ <= 0) return;

  float texel[2] = { 1.0f / static_cast<float>(width_),
                     1.0f / static_cast<float>(height_) };
  gst_gl_shader_set_uniform_2fv(shader, "u_texel_size", 1, texel);
  gst_gl_shader_set_uniform_1f(shader, "u_blur_radius", 15.0f);
  textureUniformsSet_ = true;
}

void GLRenderer::updateAmplitudes(const std::vector<float>& dBValues) {
  if (!configured_) return;

  const std::size_t count = dBValues.size();
  std::vector<float> linear(count);

  for (std::size_t i = 0; i < count; ++i) {
    linear[i] = dBtoLinear(dBValues[i]);
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingAmplitudes_ = std::move(linear);
  }
}

float GLRenderer::dBtoLinear(float dB) {
  if (dB <= -60.0f) return 0.0f;
  if (dB >= 0.0f) return 1.0f;
  return (dB + 60.0f) / 60.0f;
}

} // namespace ar_overlay
