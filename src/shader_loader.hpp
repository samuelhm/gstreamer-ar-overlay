/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shader_loader.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:04:03 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:04 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <string_view>

namespace ar_overlay {

class ShaderLoader {
public:
  ShaderLoader() = default;

  ShaderLoader(const ShaderLoader&) = delete;
  ShaderLoader& operator=(const ShaderLoader&) = delete;
  ShaderLoader(ShaderLoader&&) noexcept = default;
  ShaderLoader& operator=(ShaderLoader&&) noexcept = default;

  std::string loadVertex(std::string_view path) const;
  std::string loadFragment(std::string_view path) const;

private:
  std::string readFile(std::string_view path) const;
};

} // namespace ar_overlay
