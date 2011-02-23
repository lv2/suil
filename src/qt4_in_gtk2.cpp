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
