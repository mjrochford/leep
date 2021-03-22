project('leep', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

cc = meson.get_compiler('c')

deps = [dependency('raylib'), cc.find_library('m')]

sources = ['main.c']
executable('leep',
          dependencies: deps,
          sources: sources)
