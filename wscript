#!/usr/bin/env python
import os
import subprocess
import sys

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs, waflib.Options as Options

# Version of this package (even if built as a child)
SUIL_VERSION       = '0.4.4'
SUIL_MAJOR_VERSION = '0'

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
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    autowaf.set_options(opt)
    opt.add_option('--gtk2-lib-name', type='string', dest='gtk2_lib_name',
                   default="libgtk-x11-2.0.so",
                   help="Gtk2 library name [Default: libgtk-x11-2.0.so]")

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.line_just = 46
    autowaf.configure(conf)
    autowaf.display_header('Suil Configuration')

    conf.env.append_unique('CFLAGS', '-std=c99')

    autowaf.check_pkg(conf, 'lv2-lv2plug.in-ns-extensions-ui', uselib_store='LV2_UI')

    autowaf.check_pkg(conf, 'gtk+-2.0', uselib_store='GTK2',
                      atleast_version='2.18.0', mandatory=False)
    if not conf.env['HAVE_GTK2']:
        autowaf.check_pkg(conf, 'gtk+-2.0', uselib_store='GTK2',
                          atleast_version='2.0.0', mandatory=False)
        if conf.env['HAVE_GTK']:
            autowaf.define('SUIL_OLD_GTK', 1)

    autowaf.check_pkg(conf, 'QtGui', uselib_store='QT4',
                      atleast_version='4.0.0', mandatory=False)

    autowaf.define(conf, 'SUIL_VERSION', SUIL_VERSION)
    autowaf.define(conf, 'SUIL_MODULE_DIR',
                   conf.env['LIBDIR'] + '/suil-' + SUIL_MAJOR_VERSION)
    autowaf.define(conf, 'SUIL_DIR_SEP', '/')
    autowaf.define(conf, 'SUIL_MODULE_EXT', '.so')
    autowaf.define(conf, 'SUIL_GTK2_LIB_NAME', Options.options.gtk2_lib_name);

    conf.env['LIB_SUIL'] = ['suil-%s' % SUIL_MAJOR_VERSION]

    conf.write_config_header('suil-config.h', remove=False)

    autowaf.display_msg(conf, "Gtk2 Support",
                        conf.is_defined('HAVE_GTK2'))
    autowaf.display_msg(conf, "Gtk2 Library Name",
                        conf.env['SUIL_GTK2_LIB_NAME'])
    autowaf.display_msg(conf, "Qt4 Support",
                        conf.is_defined('HAVE_QT4'))
    print('')

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/suil-%s/suil' % SUIL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('suil/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'SUIL', SUIL_VERSION, SUIL_MAJOR_VERSION, [],
                     {'SUIL_MAJOR_VERSION' : SUIL_MAJOR_VERSION})

    cflags    = [ '-DSUIL_SHARED',
                  '-DSUIL_INTERNAL' ]
    linkflags = []
    if sys.platform != 'win32':
        cflags    += [ '-fvisibility=hidden' ]
        linkflags += [ '-ldl' ]

    module_dir = '${LIBDIR}/suil-' + SUIL_MAJOR_VERSION

    # Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = 'src/host.c src/instance.c',
              target          = 'suil-%s' % SUIL_MAJOR_VERSION,
              includes        = ['.'],
              name            = 'libsuil',
              vnum            = SUIL_LIB_VERSION,
              install_path    = '${LIBDIR}',
              cflags          = cflags,
              linkflags       = linkflags,
              uselib          = 'LV2_UI')

    if bld.is_defined('HAVE_GTK2') and bld.is_defined('HAVE_QT4'):
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/gtk2_in_qt4.cpp',
                  target       = 'suil_gtk2_in_qt4',
                  includes     = ['.'],
                  install_path = module_dir,
                  cflags       = cflags)
        autowaf.use_lib(bld, obj, 'GTK2 QT4')

        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/qt4_in_gtk2.cpp',
                  target       = 'suil_qt4_in_gtk2',
                  includes     = ['.'],
                  install_path = module_dir,
                  cflags       = cflags)
        autowaf.use_lib(bld, obj, 'GTK2 QT4')

    # Documentation
    autowaf.build_dox(bld, 'SUIL', SUIL_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env['DOCS']:
        bld.add_post_fun(fix_docs)

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def fix_docs(ctx):
    try:
        top = os.getcwd()
        os.chdir(build_dir(ctx, 'doc/html'))
        os.system("sed -i 's/SUIL_API //' group__suil.html")
        os.system("sed -i 's/SUIL_DEPRECATED //' group__suil.html")
        os.remove('index.html')
        os.symlink('group__suil.html',
                   'index.html')
        os.chdir(top)
        os.chdir(build_dir(ctx, 'doc/man/man3'))
        os.system("sed -i 's/SUIL_API //' suil.3")
        os.chdir(top)
    except:
        Logs.error("Failed to fix up %s documentation" % APPNAME)

def upload_docs(ctx):
    os.system("rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/suil/")

def lint(ctx):
    subprocess.call('cpplint.py --filter=-whitespace,+whitespace/comments,-build/header_guard,-readability/casting,-readability/todo src/* suil/*', shell=True)
