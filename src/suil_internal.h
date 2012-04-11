/*
  Copyright 2007-2012 David Robillard <http://drobilla.net>

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

#ifndef SUIL_INTERNAL_H
#define SUIL_INTERNAL_H

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define dlopen(path, flags) LoadLibrary(path)
#define dlclose(lib) FreeLibrary(lib)
#define dlsym GetProcAddress
static inline char* dlerror(void) { return "Unknown error"; }
#else
#include <dlfcn.h>
#endif

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "suil/suil.h"

#define SUIL_ERRORF(fmt, ...) fprintf(stderr, "error: %s: " fmt, \
                                      __func__, __VA_ARGS__)

struct SuilHostImpl {
	SuilPortWriteFunc    write_func;
	LV2UI_Port_Map       port_map;
	LV2UI_Port_Subscribe port_subscribe;
	LV2UI_Touch          touch;
	void*                gtk_lib;
};

struct _SuilWrapper;

typedef void (*SuilWrapperFreeFunc)(struct _SuilWrapper*);

typedef int (*SuilWrapperWrapFunc)(struct _SuilWrapper* wrapper,
                                   SuilInstance*        instance);

typedef struct _SuilWrapper {
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

typedef void (*SuilVoidFunc)();

/** dlsym wrapper to return a function pointer (without annoying warning) */
static inline SuilVoidFunc
suil_dlfunc(void* handle, const char* symbol)
{
	typedef SuilVoidFunc (*VoidFuncGetter)(void*, const char*);
	VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;
	return dlfunc(handle, symbol);
}

/** Add a feature to a (mutable) LV2 feature array. */
static inline void
suil_add_feature(LV2_Feature*** features,
                 unsigned       n,
                 const char*    uri,
                 void*          data)
{
	*features = (LV2_Feature**)realloc(*features,
	                                   sizeof(LV2_Feature*) * (n + 2));

	(*features)[n]       = (LV2_Feature*)malloc(sizeof(LV2_Feature));
	(*features)[n]->URI  = uri;
	(*features)[n]->data = data;
	(*features)[n + 1]   = NULL;
}

#endif  // SUIL_INTERNAL_H
