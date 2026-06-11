project('ar-overlay', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++20',
    'buildtype=debugoptimized',
  ])

add_project_arguments(
  '-Wpedantic',
  '-Wno-missing-field-initializers',
  language : 'cpp',
)

gst_dep = dependency('gstreamer-1.0', version : '>=1.20')
gst_gl_dep = dependency('gstreamer-gl-1.0', version : '>=1.20')
gtk4_dep = dependency('gtk4', version : '>=4.0')

executable('ar-overlay',
  'src/main.cpp',
  'src/pipeline.cpp',
  'src/spectrum.cpp',
  'src/shader_loader.cpp',
  'src/renderer.cpp',
  'src/gui.cpp',
  dependencies : [gst_dep, gst_gl_dep, gtk4_dep])
