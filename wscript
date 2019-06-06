#!/usr/bin/env python

import os
import subprocess

from waflib import TaskGen
from waflib.extras import autowaf

# Semver package/library version
SUIL_VERSION       = '0.10.4'
SUIL_MAJOR_VERSION = SUIL_VERSION[0:SUIL_VERSION.find('.')]

# Mandatory waf variables
APPNAME = 'suil'        # Package name for waf dist
VERSION = SUIL_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('compiler_cxx')
    opt = ctx.configuration_options()

    opt.add_option('--gtk2-lib-name', type='string', dest='gtk2_lib_name',
                   default="libgtk-x11-2.0.so.0",
                   help="Gtk2 library name [Default: libgtk-x11-2.0.so.0]")
    opt.add_option('--gtk3-lib-name', type='string', dest='gtk3_lib_name',
                   default="libgtk-x11-3.0.so.0",
                   help="Gtk3 library name [Default: libgtk-x11-3.0.so.0]")

    ctx.add_flags(
        opt,
        {'static':    'build static library',
         'no-shared': 'do not build shared library',
         'no-gtk':    'do not build support for Gtk',
         'no-qt':     'do not build support for Qt (any version)',
         'no-qt4':    'do not build support for Qt4',
         'no-qt5':    'do not build support for Qt5'})

def configure(conf):
    conf.load('compiler_c', cache=True)
    conf.load('compiler_cxx', cache=True)
    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')

    conf.env.BUILD_SHARED = not conf.options.no_shared
    conf.env.BUILD_STATIC = conf.options.static

    if not conf.env.BUILD_SHARED and not conf.env.BUILD_STATIC:
        conf.fatal('Neither a shared nor a static build requested')

    conf.env.NODELETE_FLAGS = []
    if (not conf.env.MSVC_COMPILER and
        conf.check(linkflags = ['-Wl,-z,nodelete'],
                   msg       = 'Checking for link flags -Wl,-z,-nodelete',
                   mandatory = False)):
        conf.env.NODELETE_FLAGS = ['-Wl,-z,nodelete']

    autowaf.check_pkg(conf, 'lv2', atleast_version='1.16.0', uselib_store='LV2')
    autowaf.check_pkg(conf, 'x11', uselib_store='X11', mandatory=False)

    if not conf.options.no_gtk:
        autowaf.check_pkg(conf, 'gtk+-2.0', uselib_store='GTK2',
                          atleast_version='2.18.0', mandatory=False)
        if not conf.env.HAVE_GTK2:
            autowaf.check_pkg(conf, 'gtk+-2.0', uselib_store='GTK2',
                              atleast_version='2.0.0', mandatory=False)
            if conf.env.HAVE_GTK2:
                autowaf.define(conf, 'SUIL_OLD_GTK', 1)

        autowaf.check_pkg(conf, 'gtk+-x11-2.0', uselib_store='GTK2_X11',
                          atleast_version='2.0.0', mandatory=False)

        autowaf.check_pkg(conf, 'gtk+-quartz-2.0', uselib_store='GTK2_QUARTZ',
                          atleast_version='2.0.0', mandatory=False)

        autowaf.check_pkg(conf, 'gtk+-3.0', uselib_store='GTK3',
                          atleast_version='3.14.0', mandatory=False)

        autowaf.check_pkg(conf, 'gtk+-x11-3.0', uselib_store='GTK3_X11',
                          atleast_version='3.14.0', mandatory=False)

    if not conf.options.no_qt:
        if not conf.options.no_qt4:
            autowaf.check_pkg(conf, 'QtGui', uselib_store='QT4',
                              atleast_version='4.4.0', mandatory=False)

        if not conf.options.no_qt5:
            autowaf.check_pkg(conf, 'Qt5Widgets', uselib_store='QT5',
                              atleast_version='5.1.0', mandatory=False)
            if conf.check_cxx(header_name = 'QMacCocoaViewContainer',
                              uselib      = 'QT5_COCOA',
                              mandatory   = False):
                autowaf.define(conf, 'SUIL_WITH_COCOA_IN_QT5', 1)

    conf.check_cc(define_name   = 'HAVE_LIBDL',
                  lib           = 'dl',
                  mandatory     = False)

    autowaf.define(conf, 'SUIL_MODULE_DIR',
                   conf.env.LIBDIR + '/suil-' + SUIL_MAJOR_VERSION)
    autowaf.define(conf, 'SUIL_DIR_SEP', '/')
    autowaf.define(conf, 'SUIL_GTK2_LIB_NAME', conf.options.gtk2_lib_name);
    autowaf.define(conf, 'SUIL_GTK3_LIB_NAME', conf.options.gtk3_lib_name);

    if conf.env.HAVE_GTK2 and conf.env.HAVE_QT4:
        autowaf.define(conf, 'SUIL_WITH_QT4_IN_GTK2', 1)
        if conf.env.HAVE_GTK2_X11:
            autowaf.define(conf, 'SUIL_WITH_GTK2_IN_QT4', 1)

    if conf.env.HAVE_GTK2 and conf.env.HAVE_QT5:
        autowaf.define(conf, 'SUIL_WITH_GTK2_IN_QT5', 1)
        autowaf.define(conf, 'SUIL_WITH_QT5_IN_GTK2', 1)

    if conf.env.HAVE_GTK2 and conf.env.HAVE_GTK2_X11:
        autowaf.define(conf, 'SUIL_WITH_X11_IN_GTK2', 1)

    if conf.env.HAVE_GTK3 and conf.env.HAVE_GTK3_X11:
        autowaf.define(conf, 'SUIL_WITH_X11_IN_GTK3', 1)

    if conf.env.HAVE_GTK3 and conf.env.HAVE_QT5:
        autowaf.define(conf, 'SUIL_WITH_QT5_IN_GTK3', 1)

    if conf.env.HAVE_GTK2 and conf.env.HAVE_GTK2_QUARTZ:
        autowaf.define(conf, 'SUIL_WITH_COCOA_IN_GTK2', 1)

    if conf.env.HAVE_GTK2 and conf.env.DEST_OS == 'win32':
        autowaf.define(conf, 'SUIL_WITH_WIN_IN_GTK2', 1)

    if conf.env.HAVE_QT4:
        autowaf.define(conf, 'SUIL_WITH_X11_IN_QT4', 1)

    if conf.env.HAVE_QT5:
        autowaf.define(conf, 'SUIL_WITH_X11_IN_QT5', 1)

    if conf.env.HAVE_X11:
        autowaf.define(conf, 'SUIL_WITH_X11', 1)

    module_prefix = ''
    module_ext    = ''
    if conf.env.PARDEBUG:
        module_ext += 'D'
    if conf.env.DEST_OS == 'win32':
        module_ext += '.dll'
    elif conf.env.DEST_OS == 'darwin':
        module_prefix = 'lib'
        module_ext += '.dylib'
    else:
        module_prefix = 'lib'
        module_ext += '.so'

    autowaf.define(conf, 'SUIL_MODULE_PREFIX', module_prefix)
    autowaf.define(conf, 'SUIL_MODULE_EXT', module_ext)

    conf.run_env.append_unique('SUIL_MODULE_DIR', [conf.build_path()])
    autowaf.set_lib_env(conf, 'suil', SUIL_VERSION)
    conf.write_config_header('suil_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {'Static library': bool(conf.env.BUILD_STATIC),
         'Shared library': bool(conf.env.BUILD_SHARED)})

    if conf.env.HAVE_GTK2:
        autowaf.display_msg(conf, "Gtk2 Library Name",
                            conf.env.SUIL_GTK2_LIB_NAME)
    if conf.env.HAVE_GTK3:
        autowaf.display_msg(conf, "Gtk3 Library Name",
                            conf.env.SUIL_GTK3_LIB_NAME)

    # Print summary message for every potentially supported wrapper
    wrappers = [('cocoa', 'gtk2'),
                ('gtk2', 'qt4'),
                ('gtk2', 'qt5'),
                ('qt4', 'gtk2'),
                ('qt5', 'gtk2'),
                ('win', 'gtk2'),
                ('x11', 'gtk2'),
                ('x11', 'gtk3'),
                ('qt5', 'gtk3'),
                ('x11', 'qt4'),
                ('x11', 'qt5'),
                ('cocoa', 'qt5')]
    for w in wrappers:
        var = 'SUIL_WITH_%s_IN_%s' % (w[0].upper(), w[1].upper())
        autowaf.display_msg(conf, 'Support for %s in %s' % (w[0], w[1]),
                            bool(conf.env[var]))

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/suil-%s/suil' % SUIL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('suil/*.h'))
    TaskGen.task_gen.mappings['.mm'] = TaskGen.task_gen.mappings['.cc']

    # Pkgconfig file
    autowaf.build_pc(bld, 'SUIL', SUIL_VERSION, SUIL_MAJOR_VERSION, [],
                     {'SUIL_MAJOR_VERSION' : SUIL_MAJOR_VERSION})

    cflags = []
    lib    = []
    modlib = []
    if bld.env.DEST_OS == 'win32':
        modlib += ['user32']
    else:
        cflags += ['-fvisibility=hidden']
        if bld.is_defined('HAVE_LIBDL'):
            lib    += ['dl']
            modlib += ['dl']

    module_dir = '${LIBDIR}/suil-' + SUIL_MAJOR_VERSION

    # Shared Library
    if bld.env.BUILD_SHARED:
        obj = bld(features        = 'c cshlib',
                  export_includes = ['.'],
                  source          = 'src/host.c src/instance.c',
                  target          = 'suil-%s' % SUIL_MAJOR_VERSION,
                  includes        = ['.'],
                  defines         = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  name            = 'libsuil',
                  vnum            = SUIL_VERSION,
                  install_path    = '${LIBDIR}',
                  cflags          = cflags,
                  lib             = lib,
                  uselib          = 'LV2')

    # Static library
    if bld.env.BUILD_STATIC:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = 'src/host.c src/instance.c',
                  target          = 'suil-%s' % SUIL_MAJOR_VERSION,
                  includes        = ['.'],
                  defines         = ['SUIL_INTERNAL'],
                  name            = 'libsuil_static',
                  vnum            = SUIL_VERSION,
                  install_path    = '${LIBDIR}',
                  cflags          = cflags,
                  lib             = lib,
                  uselib          = 'LV2')

    if bld.env.SUIL_WITH_GTK2_IN_QT4:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/gtk2_in_qt4.cpp',
                  target       = 'suil_gtk2_in_qt4',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cxxflags     = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 QT4 LV2')

    if bld.env.SUIL_WITH_GTK2_IN_QT5:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/gtk2_in_qt5.cpp',
                  target       = 'suil_gtk2_in_qt5',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cxxflags     = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 QT5 LV2')

    if bld.env.SUIL_WITH_QT4_IN_GTK2:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/qt4_in_gtk2.cpp',
                  target       = 'suil_qt4_in_gtk2',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cxxflags     = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 QT4 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_QT5_IN_GTK2:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/qt5_in_gtk.cpp',
                  target       = 'suil_qt5_in_gtk2',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cxxflags     = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 QT5 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_GTK2:
        obj = bld(features     = 'c cshlib',
                  source       = 'src/x11_in_gtk2.c',
                  target       = 'suil_x11_in_gtk2',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib + ['X11'],
                  uselib       = 'GTK2 GTK2_X11 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_GTK3:
        obj = bld(features     = 'c cshlib',
                  source       = 'src/x11_in_gtk3.c',
                  target       = 'suil_x11_in_gtk3',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib + ['X11'],
                  uselib       = 'GTK3 GTK3_X11 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_QT5_IN_GTK3:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/qt5_in_gtk.cpp',
                  target       = 'suil_qt5_in_gtk3',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'GTK3 QT5 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_COCOA_IN_GTK2:
        obj = bld(features     = 'cxx cshlib',
                  source       = 'src/cocoa_in_gtk2.mm',
                  target       = 'suil_cocoa_in_gtk2',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 LV2',
                  linkflags    = ['-framework', 'Cocoa'])

    if bld.env.SUIL_WITH_WIN_IN_GTK2:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/win_in_gtk2.cpp',
                  target       = 'suil_win_in_gtk2',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'GTK2 LV2',
                  linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_QT4:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/x11_in_qt4.cpp',
                  target       = 'suil_x11_in_qt4',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'QT4 LV2')

    if bld.env.SUIL_WITH_X11_IN_QT5:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/x11_in_qt5.cpp',
                  target       = 'suil_x11_in_qt5',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'QT5 LV2')

    if bld.env.SUIL_WITH_COCOA_IN_QT5:
        obj = bld(features     = 'cxx cxxshlib',
                  source       = 'src/cocoa_in_qt5.mm',
                  target       = 'suil_cocoa_in_qt5',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'QT5 QT5_COCOA LV2',
                  linkflags    = ['-framework', 'Cocoa'])

    if bld.env.SUIL_WITH_X11:
        obj = bld(features     = 'c cshlib',
                  source       = 'src/x11.c',
                  target       = 'suil_x11',
                  includes     = ['.'],
                  defines      = ['SUIL_SHARED', 'SUIL_INTERNAL'],
                  install_path = module_dir,
                  cflags       = cflags,
                  lib          = modlib,
                  uselib       = 'X11 LV2')

    # Documentation
    autowaf.build_dox(bld, 'SUIL', SUIL_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)

def lint(ctx):
    "checks code for style issues"
    import subprocess
    cmd = ("clang-tidy -p=. -header-filter=suil/ -checks=\"*," +
           "-clang-analyzer-alpha.*," +
           "-cppcoreguidelines-*," +
           "-google-readability-todo," +
           "-llvm-header-guard," +
           "-llvm-include-order," +
           "-misc-unused-parameters," +
           "-misc-unused-parameters," +
           "-modernize-*," +
           "-readability-else-after-return," +
           "-readability-implicit-bool-cast\" " +
           "$(find .. -name '*.c' -or -name '*.cpp' -or -name '*.mm')")
    subprocess.call(cmd, cwd='build', shell=True)

def release(ctx):
    autowaf.release(APPNAME.title(), VERSION)

def upload(ctx):
    autowaf.run_script(
        ['scp suil-%s.tar* drobilla@drobilla.net:~/download.drobilla.net/' % VERSION,
         'rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/suil/'])

def posts(ctx):
    path = str(ctx.path.abspath())
    autowaf.news_to_posts(
        os.path.join(path, 'NEWS'),
        {'title'        : 'Suil',
         'description'  : autowaf.get_blurb(os.path.join(path, 'README.md')),
         'dist_pattern' : 'http://download.drobilla.net/suil-%s.tar.bz2'},
        { 'Author' : 'drobilla',
          'Tags'   : 'Hacking, LAD, LV2, Suil' },
        os.path.join(out, 'posts'))

def dist(ctx):
    ctx.base_path = ctx.path
    ctx.excl = ctx.get_excl() + ' .gitmodules'
