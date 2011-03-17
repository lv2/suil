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

#include <gtk/gtk.h>

#include <QApplication>
#include <QX11EmbedWidget>
#include <QVBoxLayout>

#include "suil_internal.h"

extern "C" {

static int          argc = 0;
static QApplication application(argc, NULL, true);

SUIL_API
int
suil_wrap_init(const char*               host_type_uri,
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
	QX11EmbedWidget* qembed;
	SuilInstance     instance;
};

G_DEFINE_TYPE(WrapWidget, wrap_widget, GTK_TYPE_SOCKET);

static void
wrap_widget_dispose(GObject* gobject)
{
	WrapWidget* const        self = WRAP_WIDGET(gobject);
	WrapWidgetPrivate* const priv = WRAP_WIDGET_GET_PRIVATE(self);

	if (priv->qembed) {
		QWidget* const qwidget = (QWidget*)priv->instance->ui_widget;
		qwidget->setParent(NULL);

		delete self->priv->qembed;
		self->priv->qembed = NULL;
	}

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
	priv->qembed   = NULL;
	priv->instance = NULL;
	self->priv     = priv;
}

static void
wrap_widget_realize(GtkWidget* w, gpointer data)
{
	WrapWidget* const        wrap = WRAP_WIDGET(w);
	GtkSocket* const         s    = GTK_SOCKET(w);
	WrapWidgetPrivate* const priv = wrap->priv;

	gtk_socket_add_id(s, priv->qembed->winId());
	priv->qembed->show();
}

SUIL_API
int
suil_wrap(const char*  host_type_uri,
          const char*  ui_type_uri,
          SuilInstance instance)
{
	WrapWidget* const wrap = WRAP_WIDGET(g_object_new(WRAP_TYPE_WIDGET, NULL));

	WrapWidgetPrivate* const priv = wrap->priv;
	priv->qembed   = new QX11EmbedWidget();
	priv->instance = instance;

	QWidget*     qwidget = (QWidget*)instance->ui_widget;
	QVBoxLayout* layout  = new QVBoxLayout(priv->qembed);
	layout->addWidget(qwidget);

	qwidget->setParent(priv->qembed);

	g_signal_connect_after(G_OBJECT(wrap),
	                       "realize",
	                       G_CALLBACK(wrap_widget_realize),
	                       NULL);

	instance->host_widget = GTK_WIDGET(wrap);

	return 0;
}

} // extern "C"
