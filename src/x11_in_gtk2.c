/*
  Copyright 2011-2012 David Robillard <http://drobilla.net>

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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "./suil_internal.h"

#define SUIL_TYPE_X11_WRAPPER (suil_x11_wrapper_get_type())
#define SUIL_X11_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SUIL_TYPE_X11_WRAPPER, SuilX11Wrapper))

typedef struct _SuilX11Wrapper      SuilX11Wrapper;
typedef struct _SuilX11WrapperClass SuilX11WrapperClass;

struct _SuilX11Wrapper {
	GtkSocket     socket;
	GtkPlug*      plug;
	SuilWrapper*  wrapper;
	SuilInstance* instance;
};

struct _SuilX11WrapperClass {
	GtkSocketClass parent_class;
};

GType suil_x11_wrapper_get_type(void);  // Accessor for SUIL_TYPE_X11_WRAPPER

G_DEFINE_TYPE(SuilX11Wrapper, suil_x11_wrapper, GTK_TYPE_SOCKET)

static gboolean
on_plug_removed(GtkSocket* sock, gpointer data)
{
	SuilX11Wrapper* const self = SUIL_X11_WRAPPER(sock);

	if (self->instance->handle) {
		self->instance->descriptor->cleanup(self->instance->handle);
		self->instance->handle = NULL;
	}

	self->plug = NULL;
	return TRUE;
}

static void
suil_x11_wrapper_finalize(GObject* gobject)
{
	SuilX11Wrapper* const self = SUIL_X11_WRAPPER(gobject);

	self->wrapper->impl = NULL;

	G_OBJECT_CLASS(suil_x11_wrapper_parent_class)->finalize(gobject);
}

static void
suil_x11_wrapper_class_init(SuilX11WrapperClass* klass)
{
	GObjectClass* const gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = suil_x11_wrapper_finalize;
}

static void
suil_x11_wrapper_init(SuilX11Wrapper* self)
{
	self->instance = NULL;
	self->plug     = GTK_PLUG(gtk_plug_new(0));
}

static void
suil_x11_wrapper_realize(GtkWidget* w, gpointer data)
{
	SuilX11Wrapper* const wrap   = SUIL_X11_WRAPPER(w);
	GtkSocket* const      socket = GTK_SOCKET(w);

	gtk_socket_add_id(socket, gtk_plug_get_id(wrap->plug));
	gtk_widget_show_all(GTK_WIDGET(wrap->plug));
}

static int
wrapper_resize(LV2UI_Feature_Handle handle, int width, int height)
{
	gtk_widget_set_size_request(GTK_WIDGET(handle), width, height);
	return 0;
}

static int
wrapper_wrap(SuilWrapper*  wrapper,
             SuilInstance* instance)
{
	SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(wrapper->impl);

	instance->host_widget = GTK_WIDGET(wrap);
	wrap->wrapper         = wrapper;
	wrap->instance        = instance;

	g_signal_connect_after(G_OBJECT(wrap),
	                       "realize",
	                       G_CALLBACK(suil_x11_wrapper_realize),
	                       NULL);

	g_signal_connect(G_OBJECT(wrap),
	                 "plug-removed",
	                 G_CALLBACK(on_plug_removed),
	                 NULL);

	return 0;
}

static void
wrapper_free(SuilWrapper* wrapper)
{
	if (wrapper->impl) {
		SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(wrapper->impl);
		gtk_object_destroy(GTK_OBJECT(wrap));
	}
}

static GdkFilterReturn
event_filter(GdkXEvent* xevent, GdkEvent* event, gpointer data)
{
	SuilX11Wrapper* wrap = (SuilX11Wrapper*)data;
	XEvent*         ev   = (XEvent*)xevent;
	if (wrap->instance->handle && (ev->type == KeyPress || ev->type == KeyRelease)) {
		// Forward keyboard events to UI window
		XSendEvent(ev->xkey.display, (Window)wrap->instance->ui_widget, 1, 0, ev);
		XSync(ev->xkey.display, TRUE);
	}
	return GDK_FILTER_CONTINUE;
}

SUIL_LIB_EXPORT
SuilWrapper*
suil_wrapper_new(SuilHost*      host,
                 const char*    host_type_uri,
                 const char*    ui_type_uri,
                 LV2_Feature*** features,
                 unsigned       n_features)
{
	SuilWrapper* wrapper = (SuilWrapper*)malloc(sizeof(SuilWrapper));
	wrapper->wrap = wrapper_wrap;
	wrapper->free = wrapper_free;

	SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(
		g_object_new(SUIL_TYPE_X11_WRAPPER, NULL));

	wrap->wrapper = NULL;

	wrapper->impl             = wrap;
	wrapper->resize.handle    = wrap;
	wrapper->resize.ui_resize = wrapper_resize;

	GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(wrap));
	gdk_window_add_filter(window, event_filter, wrap);

	suil_add_feature(features, &n_features, LV2_UI__parent,
	                 (void*)(intptr_t)gtk_plug_get_id(wrap->plug));

	suil_add_feature(features, &n_features, LV2_UI__resize,
	                 &wrapper->resize);

	return wrapper;
}
