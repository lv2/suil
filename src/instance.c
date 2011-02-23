/* Suil, an LV2 plugin UI hosting library.
 * Copyright 2011 David Robillard <d@drobilla.net>
 *
 * Suil is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Suil is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

#include "suil-config.h"
#include "suil_internal.h"

#define NS_UI       "http://lv2plug.in/ns/extensions/ui#"
#define GTK2_UI_URI NS_UI "GtkUI"
#define QT4_UI_URI  NS_UI "Qt4UI"

SUIL_API
bool
suil_ui_type_supported(const char* host_type_uri,
                       const char* ui_type_uri)
{
	return (!strcmp(host_type_uri, GTK2_UI_URI)
	        || !strcmp(host_type_uri, QT4_UI_URI))
		&& (!strcmp(ui_type_uri, GTK2_UI_URI)
		    || !strcmp(ui_type_uri, QT4_UI_URI));
}

struct _SuilModule {
	SuilWrapInitFunc init;
	SuilWrapFunc     wrap;
};

typedef struct _SuilModule* SuilModule;

static SuilModule
get_wrap_module(const char*  host_type_uri,
                const char*  ui_type_uri)
{
	if (!strcmp(host_type_uri, ui_type_uri)) {
		return NULL;
	}

	const char* module_name = NULL;
	if (!strcmp(host_type_uri, QT4_UI_URI)
	    && !strcmp(ui_type_uri, GTK2_UI_URI)) {
		module_name = "libsuil_gtk2_in_qt4";
	} else if (!strcmp(host_type_uri, GTK2_UI_URI)
	    && !strcmp(ui_type_uri, QT4_UI_URI)) {
		module_name = "libsuil_qt4_in_gtk2";
	}

	if (!module_name) {
		SUIL_ERRORF("Unable to wrap UI type <%s> as type <%s>\n",
		            ui_type_uri, host_type_uri);
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

	free(path);
	
	return module;
}

SUIL_API
SuilInstance
suil_instance_new(SuilUIs                   uis,
                  const char*               type_uri,
                  const char*               ui_uri,
                  LV2UI_Write_Function      write_function,
                  LV2UI_Controller          controller,
                  const LV2_Feature* const* features)
{
	// Find the UI to use
	SuilUI ui = NULL;
	if (ui_uri) {
		ui = suil_uis_get(uis, ui_uri);
	} else {
		ui = suil_uis_get_best(uis, type_uri);
	}
	if (!ui) {
		SUIL_ERRORF("No suitable UI found for <%s>\n",
		            suil_uis_get_plugin_uri(uis));
		return NULL;
	}

	// Open UI library
	dlerror();
	void* lib = dlopen(ui->binary_path, RTLD_NOW);
	if (!lib) {
		SUIL_ERRORF("Unable to open UI library %s (%s)\n",
		            ui->binary_path, dlerror());
		return NULL;
	}

	// Get discovery function
	LV2UI_DescriptorFunction df = (LV2UI_DescriptorFunction)
		suil_dlfunc(lib, "lv2ui_descriptor");
	if (!df) {
		SUIL_ERRORF("Broken LV2 UI %s (no lv2ui_descriptor symbol found)\n",
		            ui->binary_path);
		dlclose(lib);
		return NULL;
	}

	// Get UI descriptor
	const LV2UI_Descriptor* descriptor = NULL;
	for (uint32_t i = 0; true; ++i) {
		const LV2UI_Descriptor* ld = df(i);
		if (!strcmp(ld->URI, ui->uri)) {
			descriptor = ld;
			break;
		}
	}
	if (!descriptor) {
		SUIL_ERRORF("Failed to find descriptor for <%s> in %s\n",
		            ui->uri, ui->binary_path);
		dlclose(lib);
		return NULL;
	}

	// Use empty local features array if necessary
	const LV2_Feature* local_features[1];
	local_features[0] = NULL;
	if (!features) {
		features = (const LV2_Feature* const*)&local_features;
	}

	SuilModule module = get_wrap_module(type_uri, ui->type_uri);
	if (module) {
		module->init(type_uri, ui->type_uri, features);
	}

	// Instantiate UI
	struct _SuilInstance* instance = malloc(sizeof(struct _SuilInstance));
	instance->lib_handle  = lib;
	instance->descriptor  = descriptor;
	instance->host_widget = NULL;
	instance->ui_widget   = NULL;
	instance->handle      = descriptor->instantiate(
		descriptor,
		uis->plugin_uri,
		ui->bundle_path,
		write_function,
		controller,
		&instance->ui_widget,
		features);

	// Failed to find or instantiate UI
	if (!instance || !instance->handle) {
		SUIL_ERRORF("Failed to instantiate UI <%s> in %s\n",
		            ui->uri, ui->binary_path);
		free(instance);
		dlclose(lib);
		return NULL;
	}

	// Got a handle, but failed to create a widget (buggy UI)
	if (!instance->ui_widget) {
		SUIL_ERRORF("Widget creation failed for UI <%s> in %s\n",
		            ui->uri, ui->binary_path);
		suil_instance_free(instance);
		return NULL;
	}

	if (module) {
		if (module->wrap(type_uri, ui->type_uri, instance)) {
			SUIL_ERRORF("Failed to wrap UI <%s> in type <%s>\n",
			            ui->uri, type_uri);
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
suil_instance_free(SuilInstance instance)
{
	if (instance) {
		instance->descriptor->cleanup(instance->handle);
		dlclose(instance->lib_handle);
		free(instance);
	}
}

SUIL_API
LV2UI_Widget
suil_instance_get_widget(SuilInstance instance)
{
	return instance->host_widget;
}

SUIL_API
void
suil_instance_port_event(SuilInstance instance,
                         uint32_t     port_index,
                         uint32_t     buffer_size,
                         uint32_t     format,
                         const void*  buffer)
{
	instance->descriptor->port_event(instance->handle,
	                                 port_index,
	                                 buffer_size,
	                                 format,
	                                 buffer);
}
