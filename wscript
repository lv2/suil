#!/usr/bin/env python

from waflib import Build, Logs, Options, TaskGen
from waflib.extras import autowaf

# Semver package/library version
SUIL_VERSION       = '0.10.11'
SUIL_MAJOR_VERSION = SUIL_VERSION[0:SUIL_VERSION.find('.')]

# Mandatory waf variables
APPNAME = 'suil'        # Package name for waf dist
VERSION = SUIL_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

# Release variables
uri          = 'http://drobilla.net/sw/suil'
dist_pattern = 'http://download.drobilla.net/suil-%d.%d.%d.tar.bz2'
post_tags    = ['Hacking', 'LAD', 'LV2', 'Suil']


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
         'no-cocoa':  'do not build support for Cocoa/Quartz',
         'no-gtk':    'do not build support for Gtk',
         'no-qt':     'do not build support for Qt (any version)',
         'no-qt5':    'do not build support for Qt5',
         'no-x11':    'do not build support for X11'})


def configure(conf):
    conf.load('compiler_c', cache=True)
    conf.load('compiler_cxx', cache=True)
    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')

    conf.env.BUILD_SHARED = not conf.options.no_shared
    conf.env.BUILD_STATIC = conf.options.static

    if not conf.env.BUILD_SHARED and not conf.env.BUILD_STATIC:
        conf.fatal('Neither a shared nor a static build requested')

    if conf.env.DOCS:
        conf.load('sphinx')

    if Options.options.strict:
        # Check for programs used by lint target
        conf.find_program("flake8", var="FLAKE8", mandatory=False)
        conf.find_program("clang-tidy", var="CLANG_TIDY", mandatory=False)
        conf.find_program("iwyu_tool", var="IWYU_TOOL", mandatory=False)

    if Options.options.ultra_strict:
        autowaf.add_compiler_flags(conf.env, '*', {
            'gcc': [
                '-Wno-padded',
                '-Wno-suggest-attribute=const',
                '-Wno-suggest-attribute=pure',
            ],
            'clang': [
                '-Wno-cast-qual',
                '-Wno-disabled-macro-expansion',
                '-Wno-padded',
            ]
        })

        autowaf.add_compiler_flags(conf.env, 'c', {
            'msvc': [
                '/wd4514',  # unreferenced inline function has been removed
                '/wd4820',  # padding added after construct
                '/wd4191',  # unsafe function conversion
                '/wd5045',  # will insert Spectre mitigation for memory load
            ],
        })

    conf.env.NODELETE_FLAGS = []
    if (not conf.env.MSVC_COMPILER and
        conf.check(linkflags = ['-Wl,-z,nodelete'],
                   msg       = 'Checking for link flags -Wl,-z,-nodelete',
                   mandatory = False)):
        conf.env.NODELETE_FLAGS = ['-Wl,-z,nodelete']

    conf.check_pkg('lv2 >= 1.16.0', uselib_store='LV2')

    if not conf.options.no_x11:
        conf.check_pkg('x11', uselib_store='X11', system=True, mandatory=False)

    def enable_module(var_name):
        conf.env[var_name] = 1

    if not conf.options.no_gtk:
        conf.check_pkg('gtk+-2.0 >= 2.18.0',
                       uselib_store='GTK2',
                       system=True,
                       mandatory=False)
        if not conf.env.HAVE_GTK2:
            conf.check_pkg('gtk+-2.0',
                           uselib_store='GTK2',
                           system=True,
                           mandatory=False)
            if conf.env.HAVE_GTK2:
                conf.define('SUIL_OLD_GTK', 1)

        if not conf.options.no_x11:
            conf.check_pkg('gtk+-x11-2.0',
                           uselib_store='GTK2_X11',
                           system=True,
                           mandatory=False)

        if not conf.options.no_cocoa:
            conf.check_pkg('gtk+-quartz-2.0',
                           uselib_store='GTK2_QUARTZ',
                           system=True,
                           mandatory=False)

        conf.check_pkg('gtk+-3.0 >= 3.14.0',
                       uselib_store='GTK3',
                       system=True,
                       mandatory=False)

        if not conf.options.no_x11:
            conf.check_pkg('gtk+-x11-3.0 >= 3.14.0',
                           uselib_store='GTK3_X11',
                           system=True,
                           mandatory=False)

    if not conf.options.no_qt:
        if not conf.options.no_qt5:
            conf.check_pkg('Qt5Widgets >= 5.1.0',
                           uselib_store='QT5',
                           system=True,
                           mandatory=False)

            if not conf.options.no_x11:
                conf.check_pkg('Qt5X11Extras >= 5.1.0',
                               uselib_store='QT5_X11',
                               system=True,
                               mandatory=False)

            if not conf.options.no_cocoa:
                if conf.check_cxx(header_name = 'QMacCocoaViewContainer',
                                  uselib      = 'QT5_COCOA',
                                  system=True,
                                  mandatory   = False):
                    enable_module('SUIL_WITH_COCOA_IN_QT5')

    conf.check_cc(define_name   = 'HAVE_LIBDL',
                  lib           = 'dl',
                  mandatory     = False)

    conf.define('SUIL_MODULE_DIR',
                conf.env.LIBDIR + '/suil-' + SUIL_MAJOR_VERSION)

    conf.define('SUIL_GTK2_LIB_NAME', conf.options.gtk2_lib_name)
    conf.define('SUIL_GTK3_LIB_NAME', conf.options.gtk3_lib_name)

    if conf.env.HAVE_GTK2 and conf.env.HAVE_QT5:
        enable_module('SUIL_WITH_GTK2_IN_QT5')
        enable_module('SUIL_WITH_QT5_IN_GTK2')

    if conf.env.HAVE_GTK2 and conf.env.HAVE_GTK2_X11:
        enable_module('SUIL_WITH_X11_IN_GTK2')

    if conf.env.HAVE_GTK3 and conf.env.HAVE_GTK3_X11:
        enable_module('SUIL_WITH_X11_IN_GTK3')

    if conf.env.HAVE_GTK3 and conf.env.HAVE_QT5:
        enable_module('SUIL_WITH_QT5_IN_GTK3')

    if conf.env.HAVE_GTK2 and conf.env.HAVE_GTK2_QUARTZ:
        enable_module('SUIL_WITH_COCOA_IN_GTK2')

    if conf.env.HAVE_GTK2 and conf.env.DEST_OS == 'win32':
        enable_module('SUIL_WITH_WIN_IN_GTK2')

    if conf.env.HAVE_QT5 and conf.env.HAVE_QT5_X11:
        enable_module('SUIL_WITH_X11_IN_QT5')

    if conf.env.HAVE_X11:
        enable_module('SUIL_WITH_X11')

    conf.run_env.append_unique('SUIL_MODULE_DIR', [conf.build_path()])

    # Set up environment for building/using as a subproject
    autowaf.set_lib_env(conf, 'suil', SUIL_VERSION,
                        include_path=str(conf.path.find_node('include')))

    conf.define('SUIL_NO_DEFAULT_CONFIG', 1)

    autowaf.display_summary(
        conf,
        {'Static library': bool(conf.env.BUILD_STATIC),
         'Shared library': bool(conf.env.BUILD_SHARED)})

    if conf.env.HAVE_GTK2:
        autowaf.display_msg(conf, "Gtk2 Library Name",
                            conf.get_define('SUIL_GTK2_LIB_NAME'))
    if conf.env.HAVE_GTK3:
        autowaf.display_msg(conf, "Gtk3 Library Name",
                            conf.get_define('SUIL_GTK3_LIB_NAME'))

    # Print summary message for every potentially supported wrapper
    wrappers = [('cocoa', 'gtk2'),
                ('gtk2', 'qt5'),
                ('qt5', 'gtk2'),
                ('win', 'gtk2'),
                ('x11', 'gtk2'),
                ('x11', 'gtk3'),
                ('qt5', 'gtk3'),
                ('x11', 'qt5'),
                ('cocoa', 'qt5')]
    for w in wrappers:
        var = 'SUIL_WITH_%s_IN_%s' % (w[0].upper(), w[1].upper())
        autowaf.display_msg(conf, 'Support for %s in %s' % (w[0], w[1]),
                            bool(conf.env[var]))


def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/suil-%s/suil' % SUIL_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('include/suil/*.h'))
    TaskGen.task_gen.mappings['.mm'] = TaskGen.task_gen.mappings['.cc']

    # Pkgconfig file
    autowaf.build_pc(bld, 'SUIL', SUIL_VERSION, SUIL_MAJOR_VERSION, [],
                     {'SUIL_MAJOR_VERSION': SUIL_MAJOR_VERSION,
                      'SUIL_PKG_DEPS': 'lv2'})

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
        bld(features        = 'c cshlib',
            export_includes = ['include'],
            source          = 'src/host.c src/instance.c',
            target          = 'suil-%s' % SUIL_MAJOR_VERSION,
            includes        = ['.', 'include'],
            defines         = ['SUIL_INTERNAL'],
            name            = 'libsuil',
            vnum            = SUIL_VERSION,
            install_path    = '${LIBDIR}',
            cflags          = cflags,
            lib             = lib,
            uselib          = 'LV2')

    # Static library
    if bld.env.BUILD_STATIC:
        bld(features        = 'c cstlib',
            export_includes = ['include'],
            source          = 'src/host.c src/instance.c',
            target          = 'suil-%s' % SUIL_MAJOR_VERSION,
            includes        = ['.', 'include'],
            defines         = ['SUIL_STATIC', 'SUIL_INTERNAL'],
            name            = 'libsuil_static',
            vnum            = SUIL_VERSION,
            install_path    = '${LIBDIR}',
            cflags          = cflags,
            lib             = lib,
            uselib          = 'LV2')

    if bld.env.SUIL_WITH_GTK2_IN_QT5:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/gtk2_in_qt5.cpp',
            target       = 'suil_gtk2_in_qt5',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cxxflags     = cflags,
            lib          = modlib,
            uselib       = 'GTK2 QT5 LV2')

    if bld.env.SUIL_WITH_QT5_IN_GTK2:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/qt5_in_gtk.cpp',
            target       = 'suil_qt5_in_gtk2',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cxxflags     = cflags,
            lib          = modlib,
            uselib       = 'GTK2 QT5 LV2',
            linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_GTK2:
        bld(features     = 'c cshlib',
            source       = 'src/x11_in_gtk2.c',
            target       = 'suil_x11_in_gtk2',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib + ['X11'],
            uselib       = 'GTK2 GTK2_X11 LV2',
            linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_GTK3:
        bld(features     = 'c cshlib',
            source       = 'src/x11_in_gtk3.c',
            target       = 'suil_x11_in_gtk3',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib + ['X11'],
            uselib       = 'GTK3 GTK3_X11 LV2',
            linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_QT5_IN_GTK3:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/qt5_in_gtk.cpp',
            target       = 'suil_qt5_in_gtk3',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'GTK3 QT5 LV2',
            linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_COCOA_IN_GTK2:
        bld(features     = 'cxx cshlib',
            source       = 'src/cocoa_in_gtk2.mm',
            target       = 'suil_cocoa_in_gtk2',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'GTK2 LV2',
            linkflags    = ['-framework', 'Cocoa'])

    if bld.env.SUIL_WITH_WIN_IN_GTK2:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/win_in_gtk2.cpp',
            target       = 'suil_win_in_gtk2',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'GTK2 LV2',
            linkflags    = bld.env.NODELETE_FLAGS)

    if bld.env.SUIL_WITH_X11_IN_QT5:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/x11_in_qt5.cpp',
            target       = 'suil_x11_in_qt5',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'QT5 QT5_X11 LV2 X11')

    if bld.env.SUIL_WITH_COCOA_IN_QT5:
        bld(features     = 'cxx cxxshlib',
            source       = 'src/cocoa_in_qt5.mm',
            target       = 'suil_cocoa_in_qt5',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'QT5 QT5_COCOA LV2',
            linkflags    = ['-framework', 'Cocoa'])

    if bld.env.SUIL_WITH_X11:
        bld(features     = 'c cshlib',
            source       = 'src/x11.c',
            target       = 'suil_x11',
            includes     = ['.', 'include'],
            defines      = ['SUIL_INTERNAL'],
            install_path = module_dir,
            cflags       = cflags,
            lib          = modlib,
            uselib       = 'X11 LV2')

    # Documentation
    if bld.env.DOCS:
        bld.recurse('doc/c')

    bld.add_post_fun(autowaf.run_ldconfig)


class LintContext(Build.BuildContext):
    fun = cmd = 'lint'


def lint(ctx):
    "checks code for style issues"
    import glob
    import os
    import subprocess
    import sys

    st = 0

    if "FLAKE8" in ctx.env:
        Logs.info("Running flake8")
        st = subprocess.call([ctx.env.FLAKE8[0],
                              "wscript",
                              "--ignore",
                              "E101,E129,W191,E221,W504,E251,E241,E741"])
    else:
        Logs.warn("Not running flake8")

    if "IWYU_TOOL" in ctx.env:
        Logs.info("Running include-what-you-use")

        qt_mapping_file = "/usr/share/include-what-you-use/qt5_11.imp"
        extra_args = []
        if os.path.exists(qt_mapping_file):
            extra_args += ["--", "-Xiwyu", "--mapping_file=" + qt_mapping_file]

        cmd = [ctx.env.IWYU_TOOL[0], "-o", "clang", "-p", "build"] + extra_args
        output = subprocess.check_output(cmd).decode('utf-8')
        if 'error: ' in output:
            sys.stdout.write(output)
            st += 1
    else:
        Logs.warn("Not running include-what-you-use")

    if "CLANG_TIDY" in ctx.env and "clang" in ctx.env.CC[0]:
        Logs.info("Running clang-tidy")
        sources = glob.glob('src/*.c') + glob.glob('tests/*.c')
        sources = list(map(os.path.abspath, sources))
        procs = []
        for source in sources:
            cmd = [ctx.env.CLANG_TIDY[0], "--quiet", "-p=.", source]
            procs += [subprocess.Popen(cmd, cwd="build")]

        for proc in procs:
            stdout, stderr = proc.communicate()
            st += proc.returncode
    else:
        Logs.warn("Not running clang-tidy")

    if st != 0:
        sys.exit(st)


def dist(ctx):
    ctx.base_path = ctx.path
    ctx.excl = ctx.get_excl() + ' .gitmodules'
