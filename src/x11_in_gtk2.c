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

#include <gtk/gtk.h>

//#include <gdk/gdkx.h>
//#include <X11/Xlib.h>

#include "suil_internal.h"

SUIL_API
int
suil_wrap_init(SuilHost*                 host,
               const char*               host_type_uri,
               const char*               ui_type_uri,
               const LV2_Feature* const* features)
{
	return 0;
}

#define SUIL_TYPE_X11_WRAPPER (suil_x11_wrapper_get_type())
#define SUIL_X11_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SUIL_TYPE_X11_WRAPPER, SuilX11Wrapper))

typedef struct _SuilX11Wrapper      SuilX11Wrapper;
typedef struct _SuilX11WrapperClass SuilX11WrapperClass;

struct _SuilX11Wrapper {
	GtkSocket     socket;
	GtkPlug*      plug;
	SuilInstance* instance;
	int           id;
};

struct _SuilX11WrapperClass {
	GtkSocketClass parent_class;
};

GType suil_x11_wrapper_get_type(void);  // Accessor for SUIL_TYPE_X11_WRAPPER

G_DEFINE_TYPE(SuilX11Wrapper, suil_x11_wrapper, GTK_TYPE_SOCKET)

static void
wrap_widget_dispose(GObject* gobject)
{
	G_OBJECT_CLASS(suil_x11_wrapper_parent_class)->dispose(gobject);
}

static void
suil_x11_wrapper_class_init(SuilX11WrapperClass* klass)
{
	GObjectClass* const gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = wrap_widget_dispose;
}

static void
suil_x11_wrapper_init(SuilX11Wrapper* self)
{
	self->instance = NULL;
	self->plug     = NULL;
	self->id       = 0;
}

static void
suil_x11_wrapper_realize(GtkWidget* w, gpointer data)
{
	SuilX11Wrapper* const wrap   = SUIL_X11_WRAPPER(w);
	GtkSocket* const      socket = GTK_SOCKET(w);

	/*
	  GdkWindow* wrap_win = GTK_WIDGET(wrap)->window;
	  Display*   display  = GDK_WINDOW_XDISPLAY(wrap_win);
	  Window     win      = wrap->id;
	  XWindowAttributes attr;
	  XGetWindowAttributes(display, win, &attr);
	  printf("WIDTH: %d HEIGHT: %d\n", attr.width, attr.height);
	*/

	gtk_socket_add_id(socket, wrap->id);
}

SUIL_API
int
suil_wrap(const char*   host_type_uri,
          const char*   ui_type_uri,
          SuilInstance* instance)
{
	SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(
		g_object_new(SUIL_TYPE_X11_WRAPPER, NULL));

	wrap->instance = instance;
	wrap->id       = (intptr_t)instance->ui_widget;

	g_signal_connect_after(G_OBJECT(wrap),
	                       "realize",
	                       G_CALLBACK(suil_x11_wrapper_realize),
	                       NULL);

	instance->host_widget = GTK_WIDGET(wrap);

	return 0;
}
