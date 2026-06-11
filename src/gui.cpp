#include "gui.hpp"

namespace ar_overlay {

GUI::GUI(int /*argc*/, char* /*argv*/[]) {
  gtk_init();
  app_ = gtk_application_new("dev.hurtadom.ar-overlay", G_APPLICATION_DEFAULT_FLAGS);
  g_application_register(G_APPLICATION(app_), nullptr, nullptr);

  g_signal_connect(app_, "activate", G_CALLBACK(+[](GtkApplication*, gpointer data) {
    auto* self = static_cast<GUI*>(data);
    gtk_window_present(GTK_WINDOW(self->window_));
  }), this);

  window_ = gtk_application_window_new(app_);
  gtk_window_set_title(GTK_WINDOW(window_), "AR Overlay");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1920, 1080);

  g_signal_connect(window_, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer) {
    g_application_quit(g_application_get_default());
  }), nullptr);
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
  g_object_unref(app_);
}

} // namespace ar_overlay
