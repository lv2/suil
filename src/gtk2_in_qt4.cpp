/*
  Copyright 2011 David Robillard <http://drobilla.net>

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
suil_wrap(const char*   host_type_uri,
          const char*   ui_type_uri,
          SuilInstance* instance)
{
	GtkWidget* const plug = gtk_plug_new(0);
	GtkWidget* const widget = (GtkWidget*)instance->ui_widget;

	gtk_container_add(GTK_CONTAINER(plug), widget);

	gtk_widget_show_all(plug);

	QX11EmbedContainer* const wrapper = new QX11EmbedContainer();
	wrapper->embedClient(gtk_plug_get_id(GTK_PLUG(plug)));

#ifdef SUIL_OLD_GTK
	wrapper->resize(widget->allocation.width, widget->allocation.height);
#else
	GtkAllocation alloc;
	gtk_widget_get_allocation(widget, &alloc);
	wrapper->resize(alloc.width, alloc.height);
#endif

	instance->host_widget = wrapper;

	return 0;
}

} // extern "C"
