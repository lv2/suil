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

#ifndef SUIL_WARNINGS_H
#define SUIL_WARNINGS_H

#if defined(__clang__)

#  define SUIL_DISABLE_GTK_WARNINGS  \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Weverything\"")

#  define SUIL_DISABLE_QT_WARNINGS   \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Weverything\"")

#  define SUIL_RESTORE_WARNINGS _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

#  if defined(__cplusplus)
#    define SUIL_DISABLE_GTK_WARNINGS                                 \
      _Pragma("GCC diagnostic push")                                  \
      _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")               \
      _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
      _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"")          \
      _Pragma("GCC diagnostic ignored \"-Wvolatile\"")                \
      _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")
#  else
#    define SUIL_DISABLE_GTK_WARNINGS                                 \
      _Pragma("GCC diagnostic push")                                  \
      _Pragma("GCC diagnostic ignored \"-Wc++-compat\"")              \
      _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")               \
      _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
      _Pragma("GCC diagnostic ignored \"-Wstrict-prototypes\"")
#  endif

#  define SUIL_DISABLE_QT_WARNINGS                               \
    _Pragma("GCC diagnostic push")                               \
    _Pragma("GCC diagnostic ignored \"-Wconversion\"")           \
    _Pragma("GCC diagnostic ignored \"-Wctor-dtor-privacy\"")    \
    _Pragma("GCC diagnostic ignored \"-Weffc++\"")               \
    _Pragma("GCC diagnostic ignored \"-Wmultiple-inheritance\"") \
    _Pragma("GCC diagnostic ignored \"-Wnoexcept\"")             \
    _Pragma("GCC diagnostic ignored \"-Wredundant-tags\"")       \
    _Pragma("GCC diagnostic ignored \"-Wsign-promo\"")           \
    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")       \
    _Pragma("GCC diagnostic ignored \"-Wabi-tag\"")              \
    _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")

#  define SUIL_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")

#else

#  define SUIL_DISABLE_GTK_WARNINGS
#  define SUIL_RESTORE_WARNINGS

#endif

#endif // SUIL_WARNINGS_H
