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

#ifndef SUIL_INTERNAL_H
#define SUIL_INTERNAL_H

#include <assert.h>
#include <stdlib.h>

#include <dlfcn.h>

#include "suil/suil.h"

#define SUIL_ERRORF(fmt, ...) fprintf(stderr, "error: %s: " fmt, \
                                      __func__, __VA_ARGS__)

struct _SuilUI {
	char* uri;
	char* type_uri;
	char* bundle_path;
	char* binary_path;
};

typedef struct _SuilUI* SuilUI;

struct _SuilUIs {
	char*    plugin_uri;
	SuilUI*  uis;
	unsigned n_uis;
};
	
struct _SuilInstance {
	void*                   lib_handle;
	const LV2UI_Descriptor* descriptor;
	LV2UI_Handle            handle;
	LV2UI_Widget            ui_widget;
	LV2UI_Widget            host_widget;
};

/** Get the UI with the given URI. */
SuilUI
suil_uis_get(SuilUIs     uis,
             const char* ui_uri);

/** Get the best UI for the given type. */
SuilUI
suil_uis_get_best(SuilUIs     uis,
                  const char* type_uri);

/** Type of a module's suil_wrap_init function.
 * This initialisation function must be called before instantiating any
 * UI that will need to be wrapped by this wrapper (e.g. it will perform any
 * initialisation required to create a widget for the given toolkit).
 */
typedef int (*SuilWrapInitFunc)(const char*               host_type_uri,
                                const char*               ui_type_uri,
                                const LV2_Feature* const* features);


typedef int (*SuilWrapFunc)(const char*  host_type_uri,
                            const char*  ui_type_uri,
                            SuilInstance instance);

typedef void (*SuilVoidFunc)();

/** dlsym wrapper to return a function pointer (without annoying warning) */
static inline SuilVoidFunc
suil_dlfunc(void* handle, const char* symbol)
{
	typedef SuilVoidFunc (*VoidFuncGetter)(void*, const char*);
	VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;
	return dlfunc(handle, symbol);
}

#endif  // SUIL_INTERNAL_H
