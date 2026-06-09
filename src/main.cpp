#include <gst/gst.h>
#include <iostream>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);
  std::cout << "GStreamer initialized\n";
  return 0;
}
