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

executable('ar-overlay',
  'src/main.cpp',
  'src/pipeline.cpp',
  'src/spectrum.cpp',
  'src/shader_loader.cpp',
  dependencies : [gst_dep])
