#!/usr/bin/env python
import os
import subprocess

import waflib.Logs as Logs, waflib.Options as Options
from waflib.extras import autowaf as autowaf

# Version of this package (even if built as a child)
SUIL_VERSION = '0.0.0'

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
	conf.line_just = max(conf.line_just, 56)
	autowaf.configure(conf)
	autowaf.display_header('Suil Configuration')

	conf.load('compiler_cc')
	conf.env.append_value('CFLAGS', '-std=c99')

	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/extensions/ui/ui.h')

	autowaf.check_pkg(conf, 'gtk+-2.0', uselib_store='GTK2',
	                  atleast_version='2.0.0', mandatory=False)

	autowaf.check_pkg(conf, 'QtGui', uselib_store='QT4',
	                  atleast_version='4.0.0', mandatory=False)

	autowaf.define(conf, 'SUIL_VERSION', SUIL_VERSION)
	autowaf.define(conf, 'SUIL_MODULE_DIR', conf.env['LIBDIR'] + '/suil')
	autowaf.define(conf, 'SUIL_DIR_SEP', '/')
	autowaf.define(conf, 'SUIL_MODULE_EXT', '.so')
	conf.write_config_header('suil-config.h', remove=False)

	autowaf.display_msg(conf, "Gtk2 Support", conf.is_defined('HAVE_GTK2'))
	autowaf.display_msg(conf, "Qt4 Support", conf.is_defined('HAVE_QT4'))
	print('')

def build(bld):
	# C Headers
	bld.install_files('${INCLUDEDIR}/suil', bld.path.ant_glob('suil/*.h'))

	# Pkgconfig file
	autowaf.build_pc(bld, 'SUIL', SUIL_VERSION, [])

	# Library
	obj = bld(features = 'c cshlib')
	obj.export_includes = ['.']
	obj.source          = 'src/instance.c'
	obj.target          = 'suil'
	obj.includes        = ['.']
	obj.name            = 'libsuil'
	obj.vnum            = SUIL_LIB_VERSION
	obj.install_path    = '${LIBDIR}'
	obj.cflags          = [ '-fvisibility=hidden', '-DSUIL_SHARED', '-DSUIL_INTERNAL' ]

	if bld.is_defined('HAVE_GTK2') and bld.is_defined('HAVE_QT4'):
		obj = bld(features = 'cxx cxxshlib')
		obj.source       = 'src/gtk2_in_qt4.cpp'
		obj.target       = 'suil_gtk2_in_qt4'
		obj.includes     = ['.']
		obj.install_path = '${LIBDIR}/suil'
		obj.cflags       = [ '-fvisibility=hidden', '-DSUIL_SHARED', '-DSUIL_INTERNAL' ]
		autowaf.use_lib(bld, obj, 'GTK2 QT4')

		obj = bld(features = 'cxx cxxshlib')
		obj.source       = 'src/qt4_in_gtk2.cpp'
		obj.target       = 'suil_qt4_in_gtk2'
		obj.includes     = ['.']
		obj.install_path = '${LIBDIR}/suil'
		obj.cflags       = [ '-fvisibility=hidden', '-DSUIL_SHARED', '-DSUIL_INTERNAL' ]
		autowaf.use_lib(bld, obj, 'GTK2 QT4')
		
	# Documentation
	autowaf.build_dox(bld, 'SUIL', SUIL_VERSION, top, out)

	bld.add_post_fun(autowaf.run_ldconfig)

def fixdocs(ctx):
    try:
        os.chdir('build/doc/html')
        os.system("sed -i 's/SUIL_API //' group__suil.html")
        os.system("sed -i 's/SUIL_DEPRECATED //' group__suil.html")
        os.remove('index.html')
        os.symlink('group__suil.html',
                   'index.html')
    except Exception as e:
        Logs.error("Failed to fix up Doxygen documentation\n")
        Logs.error(e)

def lint(ctx):
	subprocess.call('cpplint.py --filter=-whitespace,+whitespace/comments,-build/header_guard,-readability/casting,-readability/todo src/* suil/*', shell=True)
