/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shader_loader.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:04:05 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:07 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
