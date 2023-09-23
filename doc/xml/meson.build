# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

doxygen = find_program('doxygen')

config = configuration_data()
config.set('SUIL_SRCDIR', suil_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

doxyfile = configure_file(
  configuration: config,
  input: files('../Doxyfile.in'),
  output: 'Doxyfile',
)

doxygen_xml = custom_target(
  'index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [doxyfile] + c_headers,
  output: [
    'index.xml',

    'group__suil.xml',
    'group__suil__callbacks.xml',
    'group__suil__host.xml',
    'group__suil__instance.xml',
    'group__suil__library.xml',
    'suil_8h.xml',
  ],
)
