# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

api_suil_rst = custom_target(
  'suil.rst',
  command: [sphinxygen, '-f', '@INPUT0@', '@OUTDIR@'],
  input: doxygen_xml,
  output: 'suil.rst',
)
