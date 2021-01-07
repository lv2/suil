/*
  Copyright 2021 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
  Configuration header that defines reasonable defaults at compile time.

  This allows compile-time configuration from the command line (typically via
  the build system) while still allowing the source to be built without any
  configuration.  The build system can define SUIL_NO_DEFAULT_CONFIG to disable
  defaults, in which case it must define things like HAVE_FEATURE to enable
  features.  The design here ensures that compiler warnings or
  include-what-you-use will catch any mistakes.

  Note that suil loads modules at runtime, so the default configuration
  contains paths which are likely not what you want.
*/

#ifndef SUIL_CONFIG_H
#define SUIL_CONFIG_H

#if !defined(SUIL_NO_DEFAULT_CONFIG)

// X11 (requires loading an init module)
#  ifndef HAVE_X11
#    ifdef __has_include
#      if __has_include(<X11/Xlib.h>)
#        define HAVE_X11
#      endif
#    endif
#  endif

#endif // !defined(SUIL_NO_DEFAULT_CONFIG)

/*
  Make corresponding USE_FEATURE defines based on the HAVE_FEATURE defines from
  above or the command line.  The code checks for these using #if (not #ifdef),
  so there will be an undefined warning if it checks for an unknown feature,
  and this header is always required by any code that checks for features, even
  if the build system defines them all.
*/

#ifdef HAVE_X11
#  define USE_X11 1
#else
#  define USE_X11 0
#endif

/*
  Define required values.  These are always used as a fallback, even with
  LILV_NO_DEFAULT_CONFIG, since they must be defined for the build to work.
*/

// Directory of loadable wrapper modules
#ifndef SUIL_MODULE_DIR
#  ifdef _WIN32
#    define SUIL_MODULE_DIR "C:\\Program Files\\Common Files\\Suil\\lib"
#  else
#    define SUIL_MODULE_DIR "/usr/local/lib/suil-0"
#  endif
#endif

// Separator between directories in a path
#ifndef SUIL_DIR_SEP
#  define SUIL_DIR_SEP "/"
#endif

// Prefix for loadable module filenames
#ifndef SUIL_MODULE_PREFIX
#  ifdef _WIN32
#    define SUIL_MODULE_PREFIX ""
#  else
#    define SUIL_MODULE_PREFIX "lib"
#  endif
#endif

// Extension for loadable module filenames
#ifndef SUIL_MODULE_EXT
#  if defined(__APPLE__)
#    define SUIL_MODULE_EXT ".dylib"
#  elif defined(_WIN32)
#    define SUIL_MODULE_EXT ".dll"
#  else
#    define SUIL_MODULE_EXT ".so"
#  endif
#endif

// Gtk2 library to load when embedding in Qt
#ifndef SUIL_GTK2_LIB_NAME
#  define SUIL_GTK2_LIB_NAME "libgtk-x11-2.0.so.0"
#endif

// Gtk3 library to load when embedding in Qt
#ifndef SUIL_GTK3_LIB_NAME
#  define SUIL_GTK3_LIB_NAME "libgtk-x11-3.0.so.0"
#endif

#endif // SUIL_CONFIG_H
