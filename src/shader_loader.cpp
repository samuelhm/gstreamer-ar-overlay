#include "shader_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ar_overlay {

std::string ShaderLoader::loadVertex(std::string_view path) const {
  return readFile(path);
}

std::string ShaderLoader::loadFragment(std::string_view path) const {
  return readFile(path);
}

std::string ShaderLoader::readFile(std::string_view path) const {
  std::ifstream file{std::string(path)};
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open shader file: " + std::string(path));
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

} // namespace ar_overlay
