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
