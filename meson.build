# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('suil', ['c', 'cpp'],
        version: '0.10.17',
        license: 'ISC',
        meson_version: '>= 0.56.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
          'cpp_std=c++14',
        ])

suil_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'suil' + version_suffix
suil_module_dir = get_option('libdir') / versioned_name

#######################
# Compilers and Flags #
#######################

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objcpp'], native: false)
  objcpp = meson.get_compiler('objcpp')
endif

# Set global warning flags
if get_option('strict') and not meson.is_subproject()
  subdir('meson/warnings')
endif
subdir('meson/suppressions')

##########################
# Platform Configuration #
##########################

suil_abs_module_dir = get_option('prefix') / suil_module_dir
platform_defines = ['-DSUIL_MODULE_DIR="@0@"'.format(suil_abs_module_dir)]

nodelete_c_link_args = cc.get_supported_link_arguments(['-Wl,-z,nodelete'])
nodelete_cpp_link_args = cpp.get_supported_link_arguments(['-Wl,-z,nodelete'])

################
# Dependencies #
################

dl_dep = cc.find_library('dl', required: false)

lv2_dep = dependency(
  'lv2',
  version: '>= 1.18.3',
  fallback: ['lv2', 'lv2_dep'],
)

x11_dep = dependency(
  'x11',
  required: get_option('x11'),
)

gtk2_dep = dependency(
  'gtk+-2.0',
  include_type: 'system',
  version: '>=2.18.0',
  required: get_option('gtk2').enabled(),
)

gtk2_x11_dep = dependency(
  'gtk+-x11-2.0',
  include_type: 'system',
  required: get_option('gtk2').enabled() and get_option('x11').enabled(),
)

gtk2_quartz_dep = dependency(
  'gtk+-quartz-2.0',
  include_type: 'system',
  required: get_option('gtk2').enabled() and get_option('cocoa').enabled(),
)

gtk3_dep = dependency(
  'gtk+-3.0',
  include_type: 'system',
  version: '>=3.14.0',
  required: get_option('gtk3'),
)

gtk3_x11_dep = dependency(
  'gtk+-x11-3.0',
  include_type: 'system',
  version: '>=3.14.0',
  required: get_option('gtk3').enabled() and get_option('x11').enabled(),
)

qt5_dep = dependency(
  'Qt5Widgets',
  include_type: 'system',
  version: '>=5.1.0',
  required: get_option('qt5'),
)

qt5_x11_dep = dependency(
  'Qt5X11Extras',
  include_type: 'system',
  version: '>=5.1.0',
  required: get_option('qt5').enabled() and get_option('x11').enabled(),
)

if host_machine.system() == 'darwin' and not get_option('cocoa').disabled()
  objcpp.has_header(
    'QMacCocoaViewContainer',
    dependencies: qt5_dep,
    required: get_option('qt5').enabled() and get_option('cocoa').enabled(),
  )
endif

###########
# Library #
###########

include_dirs = include_directories('include')
c_headers = files('include/suil/suil.h')

sources = files(
  'src/host.c',
  'src/instance.c',
)

# Set appropriate arguments for building against the library type
extra_c_args = []
subdir('meson/library')
if get_option('default_library') == 'static'
  extra_c_args = ['-DSUIL_STATIC']
endif

# Build shared and/or static library
libsuil = library(
  meson.project_name() + library_suffix,
  sources,
  c_args: c_suppressions + extra_c_args + platform_defines + ['-DSUIL_INTERNAL'],
  dependencies: [dl_dep, lv2_dep],
  gnu_symbol_visibility: 'hidden',
  include_directories: include_dirs,
  install: true,
  version: meson.project_version(),
)

# Declare dependency for internal meson dependants
suil_dep = declare_dependency(
  compile_args: extra_c_args,
  dependencies: [dl_dep],
  include_directories: include_dirs,
  link_with: libsuil,
)

# Generage pkg-config file for external dependants
pkg.generate(
  libsuil,
  description: 'Library for loading and wrapping LV2 plugin UIs',
  extra_cflags: extra_c_args,
  filebase: versioned_name,
  name: 'Suil',
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Install header to a versioned include directory
install_headers(c_headers, subdir: versioned_name / 'suil')

#####################
# Host Init Modules #
#####################

if x11_dep.found()
  shared_module(
    'suil_x11',
    files('src/x11.c'),
    c_args: c_suppressions + platform_defines,
    dependencies: [lv2_dep, x11_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
  )
endif

###################
# Wrapper Modules #
###################

gtk_args = []
if cc.get_id() == 'clang'
  gtk_args += [
    '-Wno-reserved-identifier',
  ]
endif

gtk_c_args = cc.get_supported_arguments(gtk_args)
gtk_cpp_args = cpp.get_supported_arguments(gtk_args)

if gtk2_dep.found() and qt5_dep.found()
  shared_module(
    'suil_gtk2_in_qt5',
    files('src/gtk2_in_qt5.cpp'),
    cpp_args: cpp_suppressions + gtk_cpp_args + platform_defines,
    dependencies: [gtk2_dep, lv2_dep, qt5_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
  )

  shared_module(
    'suil_qt5_in_gtk2',
    files('src/qt5_in_gtk.cpp'),
    cpp_args: cpp_suppressions + gtk_cpp_args + platform_defines,
    dependencies: [gtk2_dep, lv2_dep, qt5_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_cpp_link_args,
  )
endif

if gtk2_dep.found() and gtk2_x11_dep.found() and x11_dep.found()
  shared_module(
    'suil_x11_in_gtk2',
    files('src/x11_in_gtk2.c'),
    c_args: c_suppressions + gtk_c_args + platform_defines,
    dependencies: [gtk2_dep, gtk2_x11_dep, lv2_dep, x11_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_c_link_args,
  )
endif

if gtk3_dep.found() and gtk3_x11_dep.found() and x11_dep.found()
  shared_module(
    'suil_x11_in_gtk3',
    files('src/x11_in_gtk3.c'),
    c_args: c_suppressions + gtk_c_args + platform_defines,
    dependencies: [gtk3_dep, gtk3_x11_dep, lv2_dep, x11_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_c_link_args,
  )
endif

if gtk3_dep.found() and gtk3_x11_dep.found() and qt5_dep.found()
  shared_module(
    'suil_qt5_in_gtk3',
    files('src/qt5_in_gtk.cpp'),
    cpp_args: cpp_suppressions + gtk_cpp_args + platform_defines,
    dependencies: [gtk3_dep, lv2_dep, qt5_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_cpp_link_args,
  )
endif

if gtk2_dep.found() and gtk2_quartz_dep.found()
  shared_module(
    'suil_cocoa_in_gtk2',
    files('src/cocoa_in_gtk2.mm'),
    dependencies: [gtk2_dep, lv2_dep, qt5_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    objcpp_args: objcpp_suppressions + gtk_cpp_args + platform_defines,
  )
endif

if gtk2_dep.found() and host_machine.system() == 'windows'
  shared_module(
    'suil_win_in_gtk2',
    files('src/win_in_gtk2.cpp'),
    cpp_args: cpp_suppressions + gtk_cpp_args + platform_defines,
    dependencies: [gtk2_dep, lv2_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_cpp_link_args,
  )
endif

if qt5_dep.found() and qt5_x11_dep.found()
  shared_module(
    'suil_x11_in_qt5',
    files('src/x11_in_qt5.cpp'),
    cpp_args: cpp_suppressions + platform_defines,
    dependencies: [lv2_dep, qt5_dep, qt5_x11_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
  )
endif

if host_machine.system() == 'darwin'
  if qt5_dep.found() and not get_option('cocoa').disabled()
    cocoa_suppressions = [
      '-Wno-deprecated-declarations',
    ]

    shared_module(
      'suil_cocoa_in_qt5',
      files('src/cocoa_in_qt5.mm'),
      dependencies: [lv2_dep, qt5_dep],
      gnu_symbol_visibility: 'hidden',
      include_directories: include_dirs,
      install: true,
      install_dir: suil_module_dir,
      objcpp_args: cocoa_suppressions + objcpp_suppressions + platform_defines,
    )
  endif
endif

#########
# Tests #
#########

if get_option('strict')
  # Check release metadata
  if not meson.is_subproject()
    autoship = find_program('autoship', required: false)
    if autoship.found()
      test('autoship', autoship, args: ['test', suil_src_root], suite: 'data')
    endif
  endif

  # Check licensing metadata
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse,
         args: ['--root', suil_src_root, 'lint'],
         suite: 'data')
  endif
endif

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
endif

if not meson.is_subproject()
  summary('Install prefix', get_option('prefix'))
  summary('Headers', get_option('prefix') / get_option('includedir'))
  summary('Libraries', get_option('prefix') / get_option('libdir'))
endif
