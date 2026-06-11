#pragma once

#include <gtk/gtk.h>

namespace ar_overlay {

class GUI {
public:
  GUI(int argc, char* argv[]);
  ~GUI() = default;

  GUI(const GUI&) = delete;
  GUI& operator=(const GUI&) = delete;

  void setVideoPaintable(GdkPaintable* paintable);
  void run();

private:
  GtkApplication* app_ = nullptr;
  GtkWidget* window_ = nullptr;
};

} // namespace ar_overlay
