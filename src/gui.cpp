#include "gui.hpp"

#include <stdexcept>

namespace ar_overlay {

GUI::GUI(int, char*[]) {
  gtk_init();

  static constexpr int kDefaultWidth = 1920;
  static constexpr int kDefaultHeight = 1080;

  app_ = gtk_application_new("dev.hurtadom.ar-overlay", G_APPLICATION_DEFAULT_FLAGS);
  if (!app_) {
    throw std::runtime_error("Failed to create GTK application");
  }

  GError* error = nullptr;
  if (!g_application_register(G_APPLICATION(app_), nullptr, &error)) {
    std::string msg = "Failed to register GTK application";
    if (error) {
      msg += std::string(": ") + error->message;
      g_error_free(error);
    }
    g_object_unref(app_);
    app_ = nullptr;
    throw std::runtime_error(msg);
  }

  g_signal_connect(app_, "activate", G_CALLBACK(+[](GtkApplication*, gpointer data) {
    auto* self = static_cast<GUI*>(data);
    gtk_window_present(GTK_WINDOW(self->window_));
  }), this);

  window_ = gtk_application_window_new(app_);
  gtk_window_set_title(GTK_WINDOW(window_), "AR Overlay");
  gtk_window_set_default_size(GTK_WINDOW(window_), kDefaultWidth, kDefaultHeight);

  g_signal_connect(window_, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer) {
    g_application_quit(g_application_get_default());
  }), nullptr);
}

GUI::~GUI() {
  if (window_) {
    gtk_widget_unparent(window_);
    window_ = nullptr;
  }
  if (app_) {
    g_object_unref(app_);
    app_ = nullptr;
  }
}

void GUI::setVideoPaintable(GdkPaintable* paintable) {
  GtkWidget* picture = gtk_picture_new_for_paintable(paintable);
  gtk_picture_set_can_shrink(GTK_PICTURE(picture), true);
  gtk_widget_set_hexpand(picture, true);
  gtk_widget_set_vexpand(picture, true);
  gtk_window_set_child(GTK_WINDOW(window_), picture);
}

void GUI::run() {
  gtk_window_present(GTK_WINDOW(window_));
  g_application_run(G_APPLICATION(app_), 0, nullptr);
}

} // namespace ar_overlay