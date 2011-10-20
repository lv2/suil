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

#define WRAP_TYPE_WIDGET (wrap_widget_get_type())
#define WRAP_WIDGET(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), WRAP_TYPE_WIDGET, WrapWidget))
#define WRAP_WIDGET_GET_PRIVATE(obj) \
	G_TYPE_INSTANCE_GET_PRIVATE((obj), \
	                            WRAP_TYPE_WIDGET, \
	                            WrapWidgetPrivate)

typedef struct _WrapWidget        WrapWidget;
typedef struct _WrapWidgetClass   WrapWidgetClass;
typedef struct _WrapWidgetPrivate WrapWidgetPrivate;

struct _WrapWidget {
  GtkSocket parent_instance;

  WrapWidgetPrivate* priv;
};

struct _WrapWidgetClass {
	GtkSocketClass parent_class;
};

GType wrap_widget_get_type(void);  // Accessor for GTK_TYPE_WIDGET

struct _WrapWidgetPrivate {
	SuilInstance*    instance;
	int              id;
};

G_DEFINE_TYPE(WrapWidget, wrap_widget, GTK_TYPE_SOCKET)

static void
wrap_widget_dispose(GObject* gobject)
{
	//WrapWidget* const        self = WRAP_WIDGET(gobject);
	//WrapWidgetPrivate* const priv = WRAP_WIDGET_GET_PRIVATE(self);

	G_OBJECT_CLASS(wrap_widget_parent_class)->dispose(gobject);
}

static void
wrap_widget_class_init(WrapWidgetClass* klass)
{
	GObjectClass* const gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = wrap_widget_dispose;

	g_type_class_add_private(klass, sizeof(WrapWidgetPrivate));
}

static void
wrap_widget_init(WrapWidget* self)
{
	WrapWidgetPrivate* const priv = WRAP_WIDGET_GET_PRIVATE(self);
	priv->instance = NULL;
	priv->id       = 0;
	self->priv     = priv;
}

static void
wrap_widget_realize(GtkWidget* w, gpointer data)
{
	WrapWidget* const        wrap = WRAP_WIDGET(w);
	GtkSocket* const         s    = GTK_SOCKET(w);
	WrapWidgetPrivate* const priv = wrap->priv;

	/*
	GdkWindow* wrap_win = GTK_WIDGET(wrap)->window;
	Display*   display  = GDK_WINDOW_XDISPLAY(wrap_win);
	Window     win      = priv->id;
	XWindowAttributes attr;
	XGetWindowAttributes(display, win, &attr);
	printf("WIDTH: %d HEIGHT: %d\n", attr.width, attr.height);
	*/

	gtk_socket_add_id(s, priv->id);
}

SUIL_API
int
suil_wrap(const char*   host_type_uri,
          const char*   ui_type_uri,
          SuilInstance* instance)
{
	WrapWidget* const wrap = WRAP_WIDGET(g_object_new(WRAP_TYPE_WIDGET, NULL));

	WrapWidgetPrivate* const priv = wrap->priv;
	priv->instance = instance;
	priv->id       = (intptr_t)instance->ui_widget;

	g_signal_connect_after(G_OBJECT(wrap),
	                       "realize",
	                       G_CALLBACK(wrap_widget_realize),
	                       NULL);

	instance->host_widget = GTK_WIDGET(wrap);

	return 0;
}
