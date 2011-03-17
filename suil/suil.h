/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
   @file slv2.h API for Suil, an LV2 UI wrapper library.
*/

#ifndef SUIL_SUIL_H
#define SUIL_SUIL_H

#include <stdbool.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#ifdef SUIL_SHARED
	#if defined _WIN32 || defined __CYGWIN__
		#define SUIL_LIB_IMPORT __declspec(dllimport)
		#define SUIL_LIB_EXPORT __declspec(dllexport)
	#else
		#define SUIL_LIB_IMPORT __attribute__ ((visibility("default")))
		#define SUIL_LIB_EXPORT __attribute__ ((visibility("default")))
	#endif
	#ifdef SUIL_INTERNAL
		#define SUIL_API SUIL_LIB_EXPORT
	#else
		#define SUIL_API SUIL_LIB_IMPORT
	#endif
#else
	#define SUIL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup suil Suil
   A library for hosting LV2 plugin UIs.
   @{
*/

/** An instance of an LV2 plugin UI. */
typedef struct _SuilInstance* SuilInstance;

/**
   Return true iff it is possible to load a UI of a given type.
   @param host_type_uri The URI of the desired widget type of the host,
   corresponding to the @a type_uri parameter of @ref suil_instance_new.
   @param ui_type_uri The URI of the UI widget type.
*/
SUIL_API
bool
suil_ui_type_supported(const char* host_type_uri,
                       const char* ui_type_uri);

/**
   Instantiate a UI for an LV2 plugin.
   @param uis Set of available UIs for the plugin.
   @param type_uri URI of the desired widget type.
   @param ui_uri URI of a specifically desired UI, or NULL to use the
   best choice given @a type_uri.
   @param write_function Write function as defined by the LV2 UI extension.
   @param controller Opaque controller to be passed to @a write_function.
   @param features NULL-terminated array of supported features, or NULL.
   @return A new UI instance, or NULL if instantiation failed.
*/
SUIL_API
SuilInstance
suil_instance_new(const char*               plugin_uri,
                  const char*               ui_uri,
                  const char*               ui_bundle_path,
                  const char*               ui_binary_path,
                  const char*               ui_type_uri,
                  const char*               host_type_uri,
                  LV2UI_Write_Function      write_function,
                  LV2UI_Controller          controller,
                  const LV2_Feature* const* features);

/**
   Free a plugin UI instance.
   The caller must ensure all references to the UI have been dropped before
   calling this function (e.g. it has been removed from its parent).
*/
SUIL_API
void
suil_instance_free(SuilInstance instance);

/**
   Get the LV2UI_Descriptor of a UI instance.
   This function should not be needed under normal circumstances.
*/
SUIL_API
const LV2UI_Descriptor*
suil_instance_get_descriptor(SuilInstance instance);

/**
   Get the LV2UI_Handle of a UI instance.
   This function should not be needed under normal circumstances.
*/
SUIL_API
LV2UI_Handle
suil_instance_get_handle(SuilInstance instance);

/**
   Get the widget for a UI instance.
   Returns an opaque pointer to a widget, the type of which is defined by the
   corresponding parameter to suil_instantiate. Note this may be a wrapper
   widget created by Suil, and not necessarily an LV2UI_Widget implemented
   in an LV2 bundle.
*/
SUIL_API
LV2UI_Widget
suil_instance_get_widget(SuilInstance instance);

/**
   Notify the UI about a change in a plugin port.
*/
SUIL_API
void
suil_instance_port_event(SuilInstance instance,
                         uint32_t     port_index,
                         uint32_t     buffer_size,
                         uint32_t     format,
                         const void*  buffer);

/**
   Return a data structure defined by some LV2 extension URI.
*/
SUIL_API
const void*
suil_instance_extension_data(SuilInstance instance,
                             const char*  uri);

/**
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SUIL_SUIL_H__ */
