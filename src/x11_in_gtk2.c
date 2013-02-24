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
suil_x11_wrapper_realize(GtkWidget* w)
{
	SuilX11Wrapper* const wrap   = SUIL_X11_WRAPPER(w);
	GtkSocket* const      socket = GTK_SOCKET(w);

	if (GTK_WIDGET_CLASS(suil_x11_wrapper_parent_class)->realize) {
		GTK_WIDGET_CLASS(suil_x11_wrapper_parent_class)->realize(w);
	}

	gtk_socket_add_id(socket, gtk_plug_get_id(wrap->plug));

	gtk_widget_set_sensitive(GTK_WIDGET(wrap->plug), TRUE);
	gtk_widget_set_can_focus(GTK_WIDGET(wrap->plug), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(wrap->plug));
}

static void
suil_x11_wrapper_show(GtkWidget* w)
{
	SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(w);

	if (GTK_WIDGET_CLASS(suil_x11_wrapper_parent_class)->show) {
		GTK_WIDGET_CLASS(suil_x11_wrapper_parent_class)->show(w);
	}

	gtk_widget_show(GTK_WIDGET(wrap->plug));
}

static void
forward_key_event(SuilX11Wrapper* socket,
                  GdkEvent*       gdk_event)
{
	GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(socket->plug));
	GdkScreen* screen = gdk_visual_get_screen(gdk_window_get_visual(window));

	XKeyEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.type      = (gdk_event->type == GDK_KEY_PRESS) ? KeyPress : KeyRelease;
	xev.root      = GDK_WINDOW_XID(gdk_screen_get_root_window(screen));
	xev.window    = GDK_WINDOW_XID(window);
	xev.subwindow = None;
	xev.time      = gdk_event->key.time;
	xev.state     = gdk_event->key.state;
	xev.keycode   = gdk_event->key.hardware_keycode;

	XSendEvent(GDK_WINDOW_XDISPLAY(window),
	           (Window)socket->instance->ui_widget,
	           False,
	           NoEventMask,
	           (XEvent*)&xev);
}

static gboolean
suil_x11_wrapper_key_event(GtkWidget*   widget,
                           GdkEventKey* event)
{
	SuilX11Wrapper* const self = SUIL_X11_WRAPPER(widget);

	if (self->plug) {
		forward_key_event(self, (GdkEvent*)event);
		return TRUE;
	}

	return FALSE;
}

static void
suil_x11_wrapper_class_init(SuilX11WrapperClass* klass)
{
	GObjectClass* const   gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass* const widget_class  = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize       = suil_x11_wrapper_finalize;
	widget_class->realize         = suil_x11_wrapper_realize;
	widget_class->show            = suil_x11_wrapper_show;
	widget_class->key_press_event = suil_x11_wrapper_key_event;
}

static void
suil_x11_wrapper_init(SuilX11Wrapper* self)
{
	self->plug     = GTK_PLUG(gtk_plug_new(0));
	self->wrapper  = NULL;
	self->instance = NULL;
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

	wrapper->impl             = wrap;
	wrapper->resize.handle    = wrap;
	wrapper->resize.ui_resize = wrapper_resize;

	gtk_widget_set_sensitive(GTK_WIDGET(wrap), TRUE);
	gtk_widget_set_can_focus(GTK_WIDGET(wrap), TRUE);

	suil_add_feature(features, &n_features, LV2_UI__parent,
	                 (void*)(intptr_t)gtk_plug_get_id(wrap->plug));

	suil_add_feature(features, &n_features, LV2_UI__resize,
	                 &wrapper->resize);

	return wrapper;
}
