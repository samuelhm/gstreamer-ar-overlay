/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gui.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: shurtado <samuel@hurtadom.dev>             +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 22:04:24 by shurtado          #+#    #+#             */
/*   Updated: 2026/06/11 22:04:25 by shurtado         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <gtk/gtk.h>

namespace ar_overlay {

class GUI {
public:
  GUI(int argc, char* argv[]);
  ~GUI();

  GUI(const GUI&) = delete;
  GUI& operator=(const GUI&) = delete;
  GUI(GUI&&) noexcept = delete;
  GUI& operator=(GUI&&) noexcept = delete;

  void setVideoPaintable(GdkPaintable* paintable);
  void run();

private:
  GtkApplication* app_ = nullptr;
  GtkWidget* window_ = nullptr;
};

} // namespace ar_overlay
