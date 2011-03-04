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

#include <QX11EmbedContainer>
#undef signals

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "suil_internal.h"

extern "C" {

SUIL_API
int
suil_wrap_init(const char*               host_type_uri,
               const char*               ui_type_uri,
               const LV2_Feature* const* features)
{
	// TODO: What Gtk initialisation is required here?
	return 0;
}

/** Dynamic module entry point. */
SUIL_API
int
suil_wrap(const char*  host_type_uri,
          const char*  ui_type_uri,
          SuilInstance instance)
{
	GtkWidget* const plug = gtk_plug_new(0);
	GtkWidget* const widget = (GtkWidget*)instance->ui_widget;

	gtk_container_add(GTK_CONTAINER(plug), widget);

	gtk_widget_show_all(plug);

	QX11EmbedContainer* const wrapper = new QX11EmbedContainer();
	wrapper->embedClient(gtk_plug_get_id(GTK_PLUG(plug)));

	GtkAllocation alloc;
	gtk_widget_get_allocation(widget, &alloc);
	wrapper->resize(alloc.width, alloc.height);

	instance->host_widget = wrapper;

	return 0;
}

} // extern "C"
