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

#include "./suil_config.h"
#include "./suil_internal.h"

extern "C" {

static int
wrapper_wrap(SuilWrapper*  wrapper,
             SuilInstance* instance)
{
	QX11EmbedContainer* const wrap   = new QX11EmbedContainer();
	GtkWidget* const          plug   = gtk_plug_new(wrap->winId());
	GtkWidget* const          widget = (GtkWidget*)instance->ui_widget;

	gtk_container_add(GTK_CONTAINER(plug), widget);
	gtk_widget_show_all(plug);

#ifdef SUIL_OLD_GTK
	wrap->resize(widget->allocation.width, widget->allocation.height);
#else
	GtkAllocation alloc;
	gtk_widget_get_allocation(widget, &alloc);
	wrap->resize(alloc.width, alloc.height);
#endif

	instance->host_widget = wrap;

	return 0;
}

SUIL_API
SuilWrapper*
suil_wrapper_new(SuilHost*      host,
                 const char*    host_type_uri,
                 const char*    ui_type_uri,
                 LV2_Feature*** features)
{
	/* We have to open libgtk here, so Gtk type symbols are present and will be
	   found by the introspection stuff.  This is required at least to make
	   GtkBuilder use in UIs work, otherwise they will cause "Invalid object
	   type" errors.
	*/
	if (!host->gtk_lib) {
		dlerror();
		host->gtk_lib = dlopen(SUIL_GTK2_LIB_NAME, RTLD_LAZY|RTLD_GLOBAL);
		if (!host->gtk_lib) {
			fprintf(stderr, "Failed to open %s (%s)\n",
			        SUIL_GTK2_LIB_NAME, dlerror());
			return NULL;
		}
		gtk_init(NULL, NULL);
	}

	SuilWrapper* wrapper = (SuilWrapper*)malloc(sizeof(SuilWrapper));
	wrapper->wrap = wrapper_wrap;
	wrapper->free = NULL;
	wrapper->impl = NULL;

	return wrapper;
}

}  // extern "C"
