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

/** @file
 * Public Suil API.
 */

#ifndef SUIL_SUIL_H__
#define SUIL_SUIL_H__

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

/** @defgroup suil Suil
 * A library for hosting LV2 plugin UIs.
 * @{
 */

/** A set of UIs for a particular LV2 plugin. */
typedef struct _SuilUIs* SuilUIs;

/** An instance of an LV2 plugin UI. */
typedef struct _SuilInstance* SuilInstance;

/** Return true iff it is possible to load a UI of a given type.
 * @param host_type_uri The URI of the desired widget type of the host,
 *        corresponding to the @a type_uri parameter of @ref suil_instance_new.
 * @param ui_type_uri The URI of the UI widget type.
 */
SUIL_API
bool
suil_ui_type_supported(const char* host_type_uri,
                       const char* ui_type_uri);

/** Create a new empty set of UIs for a particular LV2 plugin. */
SUIL_API
SuilUIs
suil_uis_new(const char* plugin_uri);

/** Free @a uis. */
SUIL_API
void
suil_uis_free(SuilUIs uis);

/** Return the URI of the plugin this set of UIs is for. */
SUIL_API
const char*
suil_uis_get_plugin_uri(SuilUIs uis);

/** Add a discovered UI to @a uis. */
SUIL_API
void
suil_uis_add(SuilUIs     uis,
             const char* uri,
             const char* type_uri,
             const char* bundle_path,
             const char* binary_path);

/** Instantiate a UI for an LV2 plugin.
 * @param uis Set of available UIs for the plugin.
 * @param type_uri URI of the desired widget type.
 * @param ui_uri URI of a specifically desired UI, or NULL to use the
 *        best choice given @a type_uri.
 * @param write_function Write function as defined by the LV2 UI extension.
 * @param controller Opaque controller to be passed to @a write_function.
 * @param features NULL-terminated array of supported features, or NULL.
 * @return A new UI instance, or NULL if instantiation failed.
 */
SUIL_API
SuilInstance
suil_instance_new(SuilUIs                   uis,
                  const char*               type_uri,
                  const char*               ui_uri,
                  LV2UI_Write_Function      write_function,
                  LV2UI_Controller          controller,
                  const LV2_Feature* const* features);

/** Free a plugin UI instance.
 * The caller must ensure all references to the UI have been dropped before
 * calling this function (e.g. it has been removed from its parent).
 */
SUIL_API
void
suil_instance_free(SuilInstance instance);

/** Get the widget for a UI instance.
 * Returns an opaque pointer to a widget, the type of which is defined by the
 * corresponding parameter to suil_instantiate. Note this may be a wrapper
 * widget created by Suil, and not necessarily an LV2UI_Widget implemented
 * in an LV2 bundle.
 */
SUIL_API
LV2UI_Widget
suil_instance_get_widget(SuilInstance instance);

/** Notify the UI about a change in a plugin port.
 */
SUIL_API
void
suil_instance_port_event(SuilInstance instance,
                         uint32_t     port_index,
                         uint32_t     buffer_size,
                         uint32_t     format,
                         const void*  buffer);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SUIL_SUIL_H__ */
