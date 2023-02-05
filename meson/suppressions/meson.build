# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#####
# C #
#####

if is_variable('cc')
  c_suppressions = []

  if get_option('strict')
    if cc.get_id() == 'clang'
      c_suppressions += [
        '-Wno-atomic-implicit-seq-cst',
        '-Wno-cast-qual',
        '-Wno-declaration-after-statement',
        '-Wno-disabled-macro-expansion',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-variadic-macros',
      ]

      if host_machine.system() == 'windows'
        c_suppressions += [
          '-Wno-deprecated-declarations',
          '-Wno-nonportable-system-include-path',
        ]
      endif

    elif cc.get_id() == 'gcc'
      c_suppressions += [
        '-Wno-padded',
        '-Wno-suggest-attribute=const',
        '-Wno-suggest-attribute=pure',
      ]

    elif cc.get_id() == 'msvc'
      c_suppressions += [
        '/wd4191', # unsafe function conversion
        '/wd4514', # unreferenced inline function has been removed
        '/wd4710', # function not inlined
        '/wd4820', # padding added after construct
        '/wd4996', # function or variable may be unsafe
        '/wd5045', # will insert Spectre mitigation for memory load
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

  if get_option('strict')
    if cpp.get_id() == 'clang'
      cpp_suppressions += [
        '-Wno-atomic-implicit-seq-cst',
        '-Wno-cast-qual',
        '-Wno-disabled-macro-expansion',
        '-Wno-old-style-cast',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-variadic-macros',
        '-Wno-zero-as-null-pointer-constant',
      ]

    elif cpp.get_id() == 'gcc'
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

  if objcpp.get_id() == 'clang'
    c_suppressions += [
      '-Wno-deprecated-declarations',
    ]
  elif objcpp.get_id() == 'gcc'
    objcpp_suppressions = gcc_common_warnings + [
      '-Wno-direct-ivar-access',
    ]
  endif

  objcpp_suppressions = objcpp.get_supported_arguments(objcpp_suppressions)
endif
