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
  int argc_ = 0;
};

} // namespace ar_overlay
