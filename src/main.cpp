#include <gst/gst.h>
#include <gtk/gtk.h>

#include <filesystem>
#include <iostream>

#include "gui.hpp"
#include "pipeline.hpp"

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <media-file>\n";
    return 1;
  }

  std::filesystem::path filePath(argv[1]);
  if (!std::filesystem::exists(filePath)) {
    std::cerr << "File not found: " << filePath << '\n';
    return 1;
  }
  if (!std::filesystem::is_regular_file(filePath)) {
    std::cerr << "Not a regular file: " << filePath << '\n';
    return 1;
  }

  try {
    ar_overlay::GUI gui(argc, argv);
    ar_overlay::Pipeline pipeline(filePath.native());

    gui.setVideoPaintable(pipeline.paintable());
    pipeline.setQuitCallback([]() {
      g_application_quit(g_application_get_default());
    });

    if (!pipeline.start()) {
      return 1;
    }

    gui.run();
  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
