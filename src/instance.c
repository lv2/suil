/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "suil-config.h"
#include "suil_internal.h"

#define NS_UI       "http://lv2plug.in/ns/extensions/ui#"
#define GTK2_UI_URI NS_UI "GtkUI"
#define QT4_UI_URI  NS_UI "Qt4UI"

SUIL_API
unsigned
suil_ui_supported(const char* container_type_uri,
                  const char* ui_type_uri)
{
	enum {
		SUIL_WRAPPING_UNSUPPORTED = 0,
		SUIL_WRAPPING_NATIVE      = 1,
		SUIL_WRAPPING_EMBEDDED    = 2
	};
	if (!strcmp(container_type_uri, ui_type_uri)) {
		return SUIL_WRAPPING_NATIVE;
	} else if ((!strcmp(container_type_uri, GTK2_UI_URI)
	            && !strcmp(ui_type_uri, QT4_UI_URI))
	           || (!strcmp(container_type_uri, QT4_UI_URI)
	               && !strcmp(ui_type_uri, GTK2_UI_URI))) {
		return SUIL_WRAPPING_EMBEDDED;
	} else {
		return SUIL_WRAPPING_UNSUPPORTED;
	}
}

struct _SuilModule {
	SuilWrapInitFunc init;
	SuilWrapFunc     wrap;
};

typedef struct _SuilModule* SuilModule;

static SuilModule
get_wrap_module(const char* container_type_uri,
                const char* ui_type_uri)
{
	if (!strcmp(container_type_uri, ui_type_uri)) {
		return NULL;
	}

	const char* module_name = NULL;
	if (!strcmp(container_type_uri, QT4_UI_URI)
	    && !strcmp(ui_type_uri, GTK2_UI_URI)) {
		module_name = "libsuil_gtk2_in_qt4";
	} else if (!strcmp(container_type_uri, GTK2_UI_URI)
	    && !strcmp(ui_type_uri, QT4_UI_URI)) {
		module_name = "libsuil_qt4_in_gtk2";
	}

	if (!module_name) {
		SUIL_ERRORF("Unable to wrap UI type <%s> as type <%s>\n",
		            ui_type_uri, container_type_uri);
		return NULL;
	}

	const size_t path_len = strlen(SUIL_MODULE_DIR)
		+ strlen(module_name)
		+ strlen(SUIL_MODULE_EXT)
		+ 2;

	char* const path = calloc(path_len, 1);
	snprintf(path, path_len, "%s%s%s%s",
	         SUIL_MODULE_DIR, SUIL_DIR_SEP, module_name, SUIL_MODULE_EXT);

	// Open wrap module
	dlerror();
	void* lib = dlopen(path, RTLD_NOW);
	if (!lib) {
		SUIL_ERRORF("Unable to open wrap module %s\n", path);
		return NULL;
	}

	SuilModule module = (SuilModule)malloc(sizeof(struct _SuilModule));
	module->init = (SuilWrapInitFunc)suil_dlfunc(lib, "suil_wrap_init");
	module->wrap = (SuilWrapFunc)suil_dlfunc(lib, "suil_wrap");

	if (!module->init || !module->wrap) {
		SUIL_ERRORF("Corrupt module %s\n", path);
		free(path);
		free(module);
		return NULL;
	}

	free(path);

	return module;
}

SUIL_API
SuilInstance*
suil_instance_new(SuilHost*                 host,
                  SuilController            controller,
                  const char*               container_type_uri,
                  const char*               plugin_uri,
                  const char*               ui_uri,
                  const char*               ui_type_uri,
                  const char*               ui_bundle_path,
                  const char*               ui_binary_path,
                  const LV2_Feature* const* features)
{
	// Open UI library
	dlerror();
	void* lib = dlopen(ui_binary_path, RTLD_NOW);
	if (!lib) {
		SUIL_ERRORF("Unable to open UI library %s (%s)\n",
		            ui_binary_path, dlerror());
		return NULL;
	}

	// Get discovery function
	LV2UI_DescriptorFunction df = (LV2UI_DescriptorFunction)
		suil_dlfunc(lib, "lv2ui_descriptor");
	if (!df) {
		SUIL_ERRORF("Broken LV2 UI %s (no lv2ui_descriptor symbol found)\n",
		            ui_binary_path);
		dlclose(lib);
		return NULL;
	}

	// Get UI descriptor
	const LV2UI_Descriptor* descriptor = NULL;
	for (uint32_t i = 0; true; ++i) {
		const LV2UI_Descriptor* ld = df(i);
		if (!strcmp(ld->URI, ui_uri)) {
			descriptor = ld;
			break;
		}
	}
	if (!descriptor) {
		SUIL_ERRORF("Failed to find descriptor for <%s> in %s\n",
		            ui_uri, ui_binary_path);
		dlclose(lib);
		return NULL;
	}

	// Use empty local features array if necessary
	const LV2_Feature* local_features[1];
	local_features[0] = NULL;
	if (!features) {
		features = (const LV2_Feature* const*)&local_features;
	}

	SuilModule module = get_wrap_module(container_type_uri, ui_type_uri);
	if (module) {
		module->init(container_type_uri, ui_type_uri, features);
	}

	// Instantiate UI
	SuilInstance* instance = malloc(sizeof(struct SuilInstanceImpl));
	instance->lib_handle  = lib;
	instance->descriptor  = descriptor;
	instance->host_widget = NULL;
	instance->ui_widget   = NULL;
	instance->handle      = descriptor->instantiate(
		descriptor,
		plugin_uri,
		ui_bundle_path,
		host->write_func,
		controller,
		&instance->ui_widget,
		features);

	// Failed to find or instantiate UI
	if (!instance || !instance->handle) {
		SUIL_ERRORF("Failed to instantiate UI <%s> in %s\n",
		            ui_uri, ui_binary_path);
		free(instance);
		dlclose(lib);
		return NULL;
	}

	// Got a handle, but failed to create a widget (buggy UI)
	if (!instance->ui_widget) {
		SUIL_ERRORF("Widget creation failed for UI <%s> in %s\n",
		            ui_uri, ui_binary_path);
		suil_instance_free(instance);
		return NULL;
	}

	if (module) {
		if (module->wrap(container_type_uri, ui_type_uri, instance)) {
			SUIL_ERRORF("Failed to wrap UI <%s> in type <%s>\n",
			            ui_uri, container_type_uri);
			suil_instance_free(instance);
			return NULL;
		}
	} else {
		instance->host_widget = instance->ui_widget;
	}

	return instance;
}

SUIL_API
void
suil_instance_free(SuilInstance* instance)
{
	if (instance) {
		instance->descriptor->cleanup(instance->handle);
		dlclose(instance->lib_handle);
		free(instance);
	}
}

SUIL_API
LV2UI_Widget
suil_instance_get_widget(SuilInstance* instance)
{
	return instance->host_widget;
}

SUIL_API
void
suil_instance_port_event(SuilInstance* instance,
                         uint32_t      port_index,
                         uint32_t      buffer_size,
                         uint32_t      format,
                         const void*   buffer)
{
	instance->descriptor->port_event(instance->handle,
	                                 port_index,
	                                 buffer_size,
	                                 format,
	                                 buffer);
}

SUIL_API
const void*
suil_instance_extension_data(SuilInstance* instance,
                             const char*   uri)
{
	return instance->descriptor->extension_data(uri);
}
