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
	return 0;
}

static void
on_window_destroy(GtkWidget* gtk_window,
                  gpointer*  user_data)
{
	//SuilInstance instance = (SuilInstance)user_data;
	// TODO: Notify host via callback
	printf("LV2 UI window destroyed\n");
}


/** Dynamic module entry point. */
SUIL_API
int
suil_instance_wrap(SuilInstance instance,
                   const char*  host_type_uri,
                   const char*  ui_type_uri)
{
	GtkWidget* gtk_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(gtk_window), TRUE);
	gtk_window_set_title(GTK_WINDOW(gtk_window), "LV2 Plugin UI");

	gtk_container_add(GTK_CONTAINER(gtk_window),
	                  (GtkWidget*)instance->ui_widget);

	g_signal_connect(G_OBJECT(gtk_window),
	                 "destroy",
	                 G_CALLBACK(on_window_destroy),
	                 instance);

	gtk_widget_show_all(gtk_window);

	GdkWindow*          gdk_window = gtk_widget_get_window(gtk_window);
	QX11EmbedContainer* wrapper    = new QX11EmbedContainer();
	wrapper->embedClient(GDK_WINDOW_XID(gdk_window));

	instance->host_widget = wrapper;

	return 0;
}

} // extern "C"
