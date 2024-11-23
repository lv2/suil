// Copyright 2007-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef SUIL_INTERNAL_H
#define SUIL_INTERNAL_H

#include "dylib.h"
#include "suil_config.h"

#include <lv2/core/lv2.h>
#include <lv2/ui/ui.h>
#include <suil/suil.h>

#ifndef _WIN32
#  include <dlfcn.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIL_ERRORF(fmt, ...) fprintf(stderr, "suil error: " fmt, __VA_ARGS__)

struct SuilHostImpl {
  SuilPortWriteFunc       write_func;
  SuilPortIndexFunc       index_func;
  SuilPortSubscribeFunc   subscribe_func;
  SuilPortUnsubscribeFunc unsubscribe_func;
  SuilTouchFunc           touch_func;
  void*                   gtk_lib;
  int                     argc;
  char**                  argv;
};

struct SuilWrapperImpl;

typedef void (*SuilWrapperFreeFunc)(struct SuilWrapperImpl*);

typedef int (*SuilWrapperWrapFunc)(struct SuilWrapperImpl* wrapper,
                                   SuilInstance*           instance);

typedef struct SuilWrapperImpl {
  SuilWrapperWrapFunc wrap;
  SuilWrapperFreeFunc free;
  void*               lib;
  void*               impl;
  LV2UI_Resize        resize;
} SuilWrapper;

struct SuilInstanceImpl {
  void*                   lib_handle;
  const LV2UI_Descriptor* descriptor;
  LV2UI_Handle            handle;
  SuilWrapper*            wrapper;
  LV2_Feature**           features;
  LV2UI_Port_Map          port_map;
  LV2UI_Port_Subscribe    port_subscribe;
  LV2UI_Touch             touch;
  SuilWidget              ui_widget;
  SuilWidget              host_widget;
};

/**
   The type of the suil_wrapper_new entry point in a wrapper module.

   This constructs a SuilWrapper which contains everything necessary
   to wrap a widget, including a possibly extended features array to
   be used for instantiating the UI.
*/
typedef SuilWrapper* (*SuilWrapperNewFunc)(SuilHost*      host,
                                           const char*    host_type_uri,
                                           const char*    ui_type_uri,
                                           LV2_Feature*** features,
                                           unsigned       n_features);

/** Prototype for suil_wrapper_new in each wrapper module. */
SUIL_LIB_EXPORT
SuilWrapper*
suil_wrapper_new(SuilHost*      host,
                 const char*    host_type_uri,
                 const char*    ui_type_uri,
                 LV2_Feature*** features,
                 unsigned       n_features);

/** Prototype for suil_host_init in each init module. */
SUIL_LIB_EXPORT
void
suil_host_init(void);

/** Dynamically load the suil module with the given name. */
static inline void*
suil_open_module(const char* module_name)
{
#define N_SLICES 4

  const char* const env_dir = getenv("SUIL_MODULE_DIR");
  const char* const mod_dir = env_dir ? env_dir : SUIL_MODULE_DIR;

  const char* const slices[N_SLICES] = {
    mod_dir, SUIL_DIR_SEP SUIL_MODULE_PREFIX, module_name, SUIL_MODULE_EXT};

  const size_t lengths[N_SLICES] = {
    strlen(slices[0]), strlen(slices[1]), strlen(slices[2]), strlen(slices[3])};

  const size_t path_len = lengths[0] + lengths[1] + lengths[2] + lengths[3];
  char* const  path     = (char*)calloc(path_len + 1, 1);

  size_t offset = 0;
  for (size_t i = 0; i < N_SLICES; ++i) {
    memcpy(path + offset, slices[i], lengths[i]);
    offset += lengths[i];
  }

  dylib_error();
  void* lib = dylib_open(path, DYLIB_NOW);
  if (!lib) {
    SUIL_ERRORF("Failed to open module %s (%s)\n", path, dylib_error());
  }

  free(path);
  return lib;

#undef N_SLICES
}

typedef void (*SuilVoidFunc)(void);

/** dlsym wrapper to return a function pointer (without annoying warning) */
static inline SuilVoidFunc
suil_dlfunc(void* handle, const char* symbol)
{
#ifdef _WIN32
  return (SuilVoidFunc)GetProcAddress((HMODULE)handle, symbol);
#else
  typedef SuilVoidFunc (*VoidFuncGetter)(void*, const char*);
  VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;
  return dlfunc(handle, symbol);
#endif
}

/** Add a feature to a (mutable) LV2 feature array. */
static inline void
suil_add_feature(LV2_Feature*** features,
                 unsigned*      n,
                 const char*    uri,
                 void*          data)
{
  for (unsigned i = 0; i < *n && (*features)[i]; ++i) {
    if (!strcmp((*features)[i]->URI, uri)) {
      (*features)[i]->data = data;
      return;
    }
  }

  *features =
    (LV2_Feature**)realloc(*features, sizeof(LV2_Feature*) * (*n + 2));

  (*features)[*n]       = (LV2_Feature*)malloc(sizeof(LV2_Feature));
  (*features)[*n]->URI  = uri;
  (*features)[*n]->data = data;
  (*features)[*n + 1]   = NULL;
  *n += 1;
}

extern int    suil_argc;
extern char** suil_argv;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // SUIL_INTERNAL_H
