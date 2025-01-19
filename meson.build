# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'suil',
  ['c', 'cpp'],
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'c_std=c99',
    'cpp_std=c++17',
  ],
  license: 'ISC',
  meson_version: '>= 0.56.0',
  version: '0.10.22',
)

suil_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'suil' + version_suffix
suil_module_dir = get_option('libdir') / versioned_name

#############################
# Compilers and Build Tools #
#############################

# Load build tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objcpp'], native: false)
  objcpp = meson.get_compiler('objcpp')
endif

########################
# Warning Suppressions #
########################

warning_level = get_option('warning_level')

# C
c_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-atomic-implicit-seq-cst',
      '-Wno-cast-function-type-strict',
      '-Wno-cast-qual',
      '-Wno-declaration-after-statement',
      '-Wno-disabled-macro-expansion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-unsafe-buffer-usage',
      '-Wno-variadic-macros',
    ]

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif
  endif

  if host_machine.system() == 'windows'
    c_suppressions += [
      '-Wno-deprecated-declarations',
      '-Wno-nonportable-system-include-path',
    ]
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=pure',
    ]
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4191', # unsafe function conversion
      '/wd4514', # unreferenced inline function has been removed
      '/wd4710', # function not inlined
      '/wd4820', # padding added after construct
      '/wd5045', # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += [
      '/wd4996', # function or variable may be unsafe
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

# C++
if is_variable('cpp')
  cpp_suppressions = []

  if cpp.get_id() in ['clang', 'emscripten']
    if warning_level == 'everything'
      cpp_suppressions += [
        '-Wno-atomic-implicit-seq-cst',
        '-Wno-c++98-compat',
        '-Wno-c++98-compat-pedantic',
        '-Wno-cast-function-type-strict',
        '-Wno-cast-qual',
        '-Wno-disabled-macro-expansion',
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-unsafe-buffer-usage',
        '-Wno-variadic-macros',
        '-Wno-zero-as-null-pointer-constant',
      ]
    endif
  elif cpp.get_id() == 'gcc'
    if warning_level == 'everything'
      cpp_suppressions += [
        '-Wno-arith-conversion',
        '-Wno-cast-qual',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-suggest-attribute=const',
        '-Wno-suggest-attribute=pure',
        '-Wno-useless-cast',
        '-Wno-volatile',
      ]
    endif
  endif

  cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
endif

# Objective C++
if is_variable('objcpp')
  objcpp_suppressions = []

  if objcpp.get_id() in ['clang', 'emscripten']
    if warning_level == 'everything'
      objcpp_suppressions += [
        '-Wno-c++98-compat-pedantic',
        '-Wno-deprecated-declarations',
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-weak-vtables',
        '-Wno-zero-as-null-pointer-constant',
      ]

      if not meson.is_cross_build()
        objcpp_suppressions += ['-Wno-poison-system-directories']
      endif
    endif

  elif objcpp.get_id() == 'gcc'
    if warning_level == 'everything'
      objcpp_suppressions = (
        gcc_common_warnings + ['-Wno-direct-ivar-access']
      )
    endif
  endif

  objcpp_suppressions = objcpp.get_supported_arguments(objcpp_suppressions)
endif

##########################
# Platform Configuration #
##########################

suil_abs_module_dir = get_option('prefix') / suil_module_dir
platform_defines = ['-DSUIL_MODULE_DIR="@0@"'.format(suil_abs_module_dir)]

nodelete_c_link_args = cc.get_supported_link_arguments(['-Wl,-z,nodelete'])
nodelete_cpp_link_args = cpp.get_supported_link_arguments(['-Wl,-z,nodelete'])

# Use versioned name everywhere to support parallel major version installations
if host_machine.system() == 'windows'
  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif
  soversion = ''
else
  soversion = meson.project_version().split('.')[0]
endif

################
# Dependencies #
################

dl_dep = cc.find_library('dl', required: false)

lv2_dep = dependency('lv2', fallback: 'lv2', version: '>= 1.18.4')

x11_dep = dependency(
  'x11',
  include_type: 'system',
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

if host_machine.system() == 'darwin'
  gtk2_quartz_dep = dependency(
    'gtk+-quartz-2.0',
    include_type: 'system',
    required: get_option('gtk2').enabled() and get_option('cocoa').enabled(),
  )
else
  gtk2_quartz_dep = disabler()
endif

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

qt6_dep = dependency(
  'Qt6Widgets',
  include_type: 'system',
  version: '>=6.2.0',
  required: get_option('qt6'),
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

core_sources = files(
  'src/host.c',
  'src/instance.c',
)

# Set appropriate arguments for building against the library type
extra_c_args = []
if get_option('default_library') == 'static'
  extra_c_args = ['-DSUIL_STATIC']
endif

# Build shared and/or static library
libsuil = library(
  versioned_name,
  core_sources,
  c_args: c_suppressions + extra_c_args + platform_defines + ['-DSUIL_INTERNAL'],
  darwin_versions: [major_version + '.0.0', meson.project_version()],
  dependencies: [dl_dep, lv2_dep],
  gnu_symbol_visibility: 'hidden',
  include_directories: include_dirs,
  install: true,
  soversion: soversion,
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
  requires: ['lv2'],
  subdirs: [versioned_name],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency(versioned_name, suil_dep)

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

if gtk2_dep.found() and gtk2_x11_dep.found() and x11_dep.found()
  shared_module(
    'suil_x11_in_gtk2',
    files('src/x11_in_gtk2.c', 'src/x11_util.c'),
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
    files('src/x11_in_gtk3.c', 'src/x11_util.c'),
    c_args: c_suppressions + gtk_c_args + platform_defines,
    dependencies: [gtk3_dep, gtk3_x11_dep, lv2_dep, x11_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
    link_args: nodelete_c_link_args,
  )
endif

if gtk2_dep.found() and gtk2_quartz_dep.found()
  shared_module(
    'suil_cocoa_in_gtk2',
    files('src/cocoa_in_gtk2.mm'),
    dependencies: [gtk2_dep, gtk2_quartz_dep, lv2_dep, qt5_dep],
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

if qt5_dep.found() and qt5_x11_dep.found() and x11_dep.found()
  shared_module(
    'suil_x11_in_qt5',
    files('src/x11_in_qt.cpp'),
    cpp_args: cpp_suppressions + platform_defines,
    dependencies: [lv2_dep, qt5_dep, qt5_x11_dep, x11_dep],
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

if qt6_dep.found()
  shared_module(
    'suil_x11_in_qt6',
    files('src/x11_in_qt.cpp'),
    cpp_args: cpp_suppressions + platform_defines,
    dependencies: [lv2_dep, qt6_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: include_dirs,
    install: true,
    install_dir: suil_module_dir,
  )
endif

########
# Lint #
########

if get_option('lint')
  all_sources = c_headers + core_sources
  all_sources += files(
    'src/cocoa_in_gtk2.mm',
    'src/cocoa_in_qt5.mm',
    'src/win_in_gtk2.cpp',
    'src/x11.c',
    'src/x11_in_gtk2.c',
    'src/x11_in_gtk3.c',
    'src/x11_in_qt.cpp',
    'src/x11_in_qt.cpp',
  )

  if not meson.is_subproject()
    # Check release metadata
    autoship = find_program('autoship', required: false)
    if autoship.found()
      test('autoship', autoship, args: ['test', suil_src_root], suite: 'data')
    endif
  endif

  # Check licensing metadata
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse, args: ['--root', suil_src_root, 'lint'], suite: 'data')
  endif

  # Check code formatting
  clang_format = find_program('clang-format', required: false)
  if clang_format.found()
    clang_format_args = ['--Werror', '--dry-run'] + all_sources
    test('format', clang_format, args: clang_format_args, suite: 'code')
  endif
endif

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
endif

# Display top-level summary (augmented in subdirectories)
if not meson.is_subproject()
  summary(
    {
      'Install prefix': get_option('prefix'),
      'Headers': get_option('prefix') / get_option('includedir'),
      'Libraries': get_option('prefix') / get_option('libdir'),
    },
    section: 'Directories',
  )
endif
