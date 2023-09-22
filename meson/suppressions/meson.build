# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions

warning_level = get_option('warning_level')

#####
# C #
#####

if is_variable('cc')
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
        c_suppressions += [
          '-Wno-poison-system-directories',
        ]
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
endif

#######
# C++ #
#######

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

#################
# Objective C++ #
#################

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
        objcpp_suppressions += [
          '-Wno-poison-system-directories',
        ]
      endif
    endif

  elif objcpp.get_id() == 'gcc'
    if warning_level == 'everything'
      objcpp_suppressions = gcc_common_warnings + [
        '-Wno-direct-ivar-access',
      ]
    endif
  endif

  objcpp_suppressions = objcpp.get_supported_arguments(objcpp_suppressions)
endif
