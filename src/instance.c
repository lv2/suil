/* Suil, a lightweight RDF syntax library.
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

#include "suil_internal.h"

SUIL_API
bool
suil_ui_type_supported(const char* uri)
{
	return !strcmp(uri, "http://lv2plug.in/ns/extensions/ui#GtkUI");
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
	struct _SuilInstance* instance = NULL;

	const bool local_features = (features == NULL);
	if (local_features) {
		features = malloc(sizeof(LV2_Feature));
		((LV2_Feature**)features)[0] = NULL;
	}

	SuilUI ui = uis->uis[0];  // FIXME

	dlerror();
	void* lib = dlopen(ui->binary_path, RTLD_NOW);
	if (!lib) {
		SUIL_ERRORF("Unable to open UI library %s (%s)\n",
		            ui->binary_path, dlerror());
		return NULL;
	}

	LV2UI_DescriptorFunction df = (LV2UI_DescriptorFunction)
		suil_dlfunc(lib, "lv2ui_descriptor");

	if (!df) {
		SUIL_ERRORF("Broken LV2 UI %s (no lv2ui_descriptor symbol found)\n",
		            ui->binary_path);
		dlclose(lib);
		return NULL;
	} else {
		for (uint32_t i = 0; true; ++i) {
			const LV2UI_Descriptor* ld = df(i);
			if (!ld) {
				SUIL_ERRORF("No UI %s in %s\n", ui->uri, ui->binary_path);
				dlclose(lib);
				break;  // return NULL
			} else if ((ui_uri && !strcmp(ld->URI, ui_uri))
			           || !strcmp(ui->type_uri, type_uri)) {
				instance             = malloc(sizeof(struct _SuilInstance));
				instance->descriptor = ld;
				instance->handle     = ld->instantiate(
					ld,
					uis->plugin_uri,
					ui->bundle_path,
					write_function,
					controller,
					&instance->widget,
					features);
				instance->lib_handle = lib;
				break;
			}
		}
	}

	if (local_features) {
		free((LV2_Feature**)features);
	}

	// Failed to find or instantiate UI
	if (!instance || !instance->handle) {
		free(instance);
		return NULL;
	}

	// Failed to create a widget, but still got a handle (buggy UI)
	if (!instance->widget) {
		suil_instance_free(instance);
		return NULL;
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
	return instance->widget;
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
