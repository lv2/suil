// Copyright 2020-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef SUIL_DYLIB_H
#define SUIL_DYLIB_H

#ifdef _WIN32

#  include <windows.h>

enum DylibFlags {
  DYLIB_GLOBAL = 0,
  DYLIB_LAZY   = 1,
  DYLIB_NOW    = 2,
};

static inline void*
dylib_open(const char* const filename, const int flags)
{
  (void)flags;
  return LoadLibrary(filename);
}

static inline int
dylib_close(void* const handle)
{
  return !FreeLibrary((HMODULE)handle);
}

static inline const char*
dylib_error(void)
{
  return "Unknown error";
}

#else

#  include <dlfcn.h>

enum DylibFlags {
  DYLIB_GLOBAL = RTLD_GLOBAL,
  DYLIB_LAZY   = RTLD_LAZY,
  DYLIB_NOW    = RTLD_NOW,
};

static inline void*
dylib_open(const char* const filename, const int flags)
{
  return dlopen(filename, flags);
}

static inline int
dylib_close(void* const handle)
{
  return dlclose(handle);
}

static inline const char*
dylib_error(void)
{
  return dlerror();
}

#endif

#endif // SUIL_DYLIB_H
