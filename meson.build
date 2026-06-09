project('ar-overlay', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
    'buildtype=debugoptimized',
  ])

gst_dep = dependency('gstreamer-1.0', version : '>=1.20')

executable('ar-overlay',
  'src/main.cpp',
  dependencies : [gst_dep])
