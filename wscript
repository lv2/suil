#!/usr/bin/env python
import Logs
import Options
import autowaf
import filecmp
import glob
import os
import shutil
import subprocess

# Version of this package (even if built as a child)
SUIL_VERSION = '0.1.0'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Suil uses the same version number for both library and package
SUIL_LIB_VERSION = SUIL_VERSION

# Variables for 'waf dist'
APPNAME = 'suil'
VERSION = SUIL_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
	autowaf.set_options(opt)

def configure(conf):
	conf.line_just = max(conf.line_just, 59)
	autowaf.configure(conf)
	autowaf.display_header('Suil Configuration')

	conf.check_tool('compiler_cc')
	conf.env.append_value('CFLAGS', '-std=c99')

	autowaf.define(conf, 'SUIL_VERSION', SUIL_VERSION)
	conf.write_config_header('suil-config.h', remove=False)

	print

def build(bld):
	# C Headers
	bld.install_files('${INCLUDEDIR}/suil', bld.path.ant_glob('suil/*.h'))

	# Pkgconfig file
	autowaf.build_pc(bld, 'SUIL', SUIL_VERSION, [])

	# Library
	obj = bld(features = 'c cshlib')
	obj.export_includes = ['.']
	obj.source          = 'src/suil.c'
	obj.includes        = ['.', './src']
	obj.name            = 'libsuil'
	obj.target          = 'suil'
	obj.vnum            = SUIL_LIB_VERSION
	obj.install_path    = '${LIBDIR}'
	obj.cflags          = [ '-fvisibility=hidden', '-DSUIL_SHARED', '-DSUIL_INTERNAL' ]

	# Documentation
	autowaf.build_dox(bld, 'SUIL', SUIL_VERSION, top, out)

	# Man page
	bld.install_files('${MANDIR}/man1', 'doc/suili.1')

	bld.add_post_fun(autowaf.run_ldconfig)

def lint(ctx):
	subprocess.call('cpplint.py --filter=-whitespace,+whitespace/comments,-build/header_guard,-readability/casting,-readability/todo src/* suil/*', shell=True)
