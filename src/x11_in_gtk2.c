// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "suil_internal.h"
#include "warnings.h"
#include "x11_util.h"

#include <lv2/core/lv2.h>
#include <lv2/options/options.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include <suil/suil.h>

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

SUIL_DISABLE_GTK_WARNINGS
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>
SUIL_RESTORE_WARNINGS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GtkSocket                   socket;
  GtkPlug*                    plug;
  SuilWrapper*                wrapper;
  SuilInstance*               instance;
  const LV2UI_Idle_Interface* idle_iface;
  guint                       idle_id;
  guint                       idle_ms;
  XSizeHints                  size_hints;
  gboolean                    size_hints_dirty;
} SuilX11Wrapper;

typedef struct {
  GtkSocketClass parent_class;
} SuilX11WrapperClass;

GType
suil_x11_wrapper_get_type(void); // Accessor for SUIL_TYPE_X11_WRAPPER

#define SUIL_TYPE_X11_WRAPPER (suil_x11_wrapper_get_type())
#define SUIL_X11_WRAPPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SUIL_TYPE_X11_WRAPPER, SuilX11Wrapper))

G_DEFINE_TYPE(SuilX11Wrapper, suil_x11_wrapper, GTK_TYPE_SOCKET)

static gboolean
on_plug_removed(GtkSocket* sock, gpointer data)
{
  (void)data;

  SuilX11Wrapper* const self = SUIL_X11_WRAPPER(sock);

  if (self->idle_id) {
    g_source_remove(self->idle_id);
    self->idle_id = 0;
  }

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

  // Setup drag/drop proxy from parent/grandparent window
  GdkWindow* gwindow         = gtk_widget_get_window(GTK_WIDGET(wrap->plug));
  Display*   xdisplay        = GDK_WINDOW_XDISPLAY(gwindow);
  Window     xwindow         = GDK_WINDOW_XID(gwindow);
  Atom       xdnd_proxy_atom = gdk_x11_get_xatom_by_name("XdndProxy");
  Window     ui_window       = (Window)wrap->instance->ui_widget;

  while (xwindow) {
    XChangeProperty(xdisplay,
                    xwindow,
                    xdnd_proxy_atom,
                    XA_WINDOW,
                    32,
                    PropModeReplace,
                    (unsigned char*)&ui_window,
                    1);

    xwindow = suil_x11_get_parent(xdisplay, xwindow);
  }
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

static gboolean
forward_key_event(SuilX11Wrapper* socket, GdkEvent* gdk_event)
{
  GdkWindow* gwindow = gtk_widget_get_window(GTK_WIDGET(socket->plug));
  GdkScreen* screen  = gdk_visual_get_screen(gdk_window_get_visual(gwindow));

  Window target_window = 0;
  if (gdk_event->any.window == gwindow) {
    // Event sent up to the plug window, forward it up to the parent
    GtkWidget* widget = GTK_WIDGET(socket->instance->host_widget);
    GdkWindow* parent = gtk_widget_get_parent_window(widget);
    if (parent) {
      target_window = GDK_WINDOW_XID(parent);
    } else {
      return FALSE; // Wrapper is a top-level window, do nothing
    }
  } else {
    // Event sent anywhere else, send to the plugin
    target_window = (Window)socket->instance->ui_widget;
  }

  XKeyEvent xev;
  memset(&xev, 0, sizeof(xev));
  xev.type      = (gdk_event->type == GDK_KEY_PRESS) ? KeyPress : KeyRelease;
  xev.root      = GDK_WINDOW_XID(gdk_screen_get_root_window(screen));
  xev.window    = target_window;
  xev.subwindow = None;
  xev.time      = gdk_event->key.time;
  xev.state     = gdk_event->key.state;
  xev.keycode   = gdk_event->key.hardware_keycode;

  XSendEvent(GDK_WINDOW_XDISPLAY(gwindow),
             target_window,
             False,
             NoEventMask,
             (XEvent*)&xev);

  return (gdk_event->any.window != gwindow);
}

static gboolean
idle_size_request(gpointer user_data)
{
  GtkWidget* w = GTK_WIDGET(user_data);
  gtk_widget_queue_resize(w);
  return FALSE;
}

/// Read XSizeHints and store the values for later use
static void
query_wm_hints(SuilX11Wrapper* wrap)
{
  GdkWindow* gwindow  = gtk_widget_get_window(GTK_WIDGET(wrap->plug));
  long       supplied = 0;

  XGetWMNormalHints(GDK_WINDOW_XDISPLAY(gwindow),
                    (Window)wrap->instance->ui_widget,
                    &wrap->size_hints,
                    &supplied);

  wrap->size_hints.flags &= ~USSize; // Reused for "custom" size
  wrap->size_hints_dirty = FALSE;
}

static void
forward_size_request(SuilX11Wrapper* socket, GtkAllocation* allocation)
{
  GdkWindow* gwindow   = gtk_widget_get_window(GTK_WIDGET(socket->plug));
  Display*   xdisplay  = GDK_WINDOW_XDISPLAY(gwindow);
  Window     ui_window = (Window)socket->instance->ui_widget;
  if (suil_x11_is_valid_child(xdisplay, GDK_WINDOW_XID(gwindow), ui_window)) {
    // Calculate allocation size constrained to X11 limits for widget
    int width  = allocation->width;
    int height = allocation->height;

    if (socket->size_hints_dirty) {
      query_wm_hints(socket);
    }

    if (socket->size_hints.flags & PMaxSize) {
      width  = MIN(width, socket->size_hints.max_width);
      height = MIN(height, socket->size_hints.max_height);
    }

    if (socket->size_hints.flags & PMinSize) {
      width  = MAX(width, socket->size_hints.min_width);
      height = MAX(height, socket->size_hints.min_height);
    }

    // Resize widget window
    XResizeWindow(xdisplay, ui_window, (unsigned)width, (unsigned)height);

    // Get actual widget geometry
    Window       root    = 0;
    int          wx      = 0;
    int          wy      = 0;
    unsigned int ww      = 0;
    unsigned int wh      = 0;
    unsigned int ignored = 0;
    XGetGeometry(
      xdisplay, ui_window, &root, &wx, &wy, &ww, &wh, &ignored, &ignored);

    // Center widget in allocation
    wx = (allocation->width - (int)ww) / 2;
    wy = (allocation->height - (int)wh) / 2;
    XMoveWindow(xdisplay, ui_window, wx, wy);
  } else {
    /* Child has not been realized, so unable to resize now.
       Queue an idle resize. */
    g_idle_add(idle_size_request, socket->plug);
  }
}

static gboolean
suil_x11_wrapper_key_event(GtkWidget* widget, GdkEventKey* event)
{
  SuilX11Wrapper* const self = SUIL_X11_WRAPPER(widget);

  if (self->plug) {
    return forward_key_event(self, (GdkEvent*)event);
  }

  return FALSE;
}

static void
suil_x11_on_size_request(GtkWidget* widget, GtkRequisition* requisition)
{
  SuilX11Wrapper* const self = SUIL_X11_WRAPPER(widget);

  if (self->size_hints.flags & USSize) {
    requisition->width  = self->size_hints.width;
    requisition->height = self->size_hints.height;
  } else if (self->size_hints.flags & PBaseSize) {
    requisition->width  = self->size_hints.base_width;
    requisition->height = self->size_hints.base_height;
  } else if (self->size_hints.flags & PMinSize) {
    requisition->width  = self->size_hints.min_width;
    requisition->height = self->size_hints.min_height;
  }
}

static void
suil_x11_on_size_allocate(GtkWidget* widget, GtkAllocation* a)
{
  SuilX11Wrapper* const self = SUIL_X11_WRAPPER(widget);

  if (self->plug && GTK_WIDGET_REALIZED(widget) && GTK_WIDGET_MAPPED(widget) &&
      GTK_WIDGET_VISIBLE(widget)) {
    forward_size_request(self, a);
  }
}

static void
suil_x11_on_map_event(GtkWidget* widget, GdkEvent* event)
{
  (void)event;

  SuilX11Wrapper* const self = SUIL_X11_WRAPPER(widget);

  /* Reset the size request to the minimum sizes.  This is called after the
     initial size negotiation, where Gtk called suil_x11_on_size_request() to
     get the size request, which might be bigger than the minimum size.
     However, the Gtk2 size model has no proper way to handle minimum and
     default sizes, so hack around this by setting the size request
     properties (which really mean minimum size) back to the minimum after
     the widget is mapped.  This makes it possible for the initial mapping to
     use the default size, but still allow the user to resize the widget
     smaller, down to the minimum size. */

  if ((self->size_hints.flags & (USSize | PBaseSize)) &&
      (self->size_hints.flags & PMinSize)) {
    g_object_set(G_OBJECT(GTK_WIDGET(self)),
                 "width-request",
                 self->size_hints.min_width,
                 "height-request",
                 self->size_hints.min_height,
                 NULL);
  }
}

static void
suil_x11_wrapper_class_init(SuilX11WrapperClass* klass)
{
  GObjectClass* const   gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass* const widget_class  = GTK_WIDGET_CLASS(klass);

  gobject_class->finalize         = suil_x11_wrapper_finalize;
  widget_class->realize           = suil_x11_wrapper_realize;
  widget_class->show              = suil_x11_wrapper_show;
  widget_class->key_press_event   = suil_x11_wrapper_key_event;
  widget_class->key_release_event = suil_x11_wrapper_key_event;
}

static void
suil_x11_wrapper_init(SuilX11Wrapper* self)
{
  self->plug             = GTK_PLUG(gtk_plug_new(0));
  self->wrapper          = NULL;
  self->instance         = NULL;
  self->idle_iface       = NULL;
  self->idle_ms          = 1000 / 30; // 30 Hz default
  self->size_hints_dirty = TRUE;

  memset(&self->size_hints, 0, sizeof(self->size_hints));
}

static int
wrapper_resize(LV2UI_Feature_Handle handle, int width, int height)
{
  SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(handle);

  wrap->size_hints.width  = width;
  wrap->size_hints.height = height;
  if (width > 0 && height > 0) {
    wrap->size_hints.flags |= USSize;
  }

  // Fetch hints to get size constraints (probably) updated by plugin
  wrap->size_hints_dirty = TRUE;

  gtk_widget_queue_resize(GTK_WIDGET(handle));
  return 0;
}

static gboolean
suil_x11_wrapper_idle(void* data)
{
  SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(data);

  wrap->idle_iface->idle(wrap->instance->handle);

  return TRUE; // Continue calling
}

static int
wrapper_wrap(SuilWrapper* wrapper, SuilInstance* instance)
{
  SuilX11Wrapper* const wrap = SUIL_X11_WRAPPER(wrapper->impl);

  instance->host_widget = GTK_WIDGET(wrap);
  wrap->wrapper         = wrapper;
  wrap->instance        = instance;

  GdkWindow* gwindow   = gtk_widget_get_window(GTK_WIDGET(wrap->plug));
  Display*   xdisplay  = GDK_WINDOW_XDISPLAY(gwindow);
  Window     ui_window = (Window)instance->ui_widget;

  gdk_display_sync(gdk_window_get_display(gwindow));
  if (suil_x11_is_valid_child(xdisplay, GDK_WINDOW_XID(gwindow), ui_window)) {
    XWindowAttributes attrs;
    XGetWindowAttributes(xdisplay, ui_window, &attrs);

    query_wm_hints(wrap);
    if (!(wrap->size_hints.flags & PBaseSize)) {
      // Fall back to using initial size as base size
      wrap->size_hints.flags |= PBaseSize;
      wrap->size_hints.base_width  = attrs.width;
      wrap->size_hints.base_height = attrs.height;
    }
  }

  const LV2UI_Idle_Interface* idle_iface = NULL;
  if (instance->descriptor->extension_data) {
    idle_iface =
      (const LV2UI_Idle_Interface*)instance->descriptor->extension_data(
        LV2_UI__idleInterface);
  }

  if (idle_iface) {
    wrap->idle_iface = idle_iface;
    wrap->idle_id = g_timeout_add(wrap->idle_ms, suil_x11_wrapper_idle, wrap);
  }

  g_signal_connect(
    G_OBJECT(wrap), "plug-removed", G_CALLBACK(on_plug_removed), NULL);

  g_signal_connect(
    G_OBJECT(wrap), "size-request", G_CALLBACK(suil_x11_on_size_request), NULL);

  g_signal_connect(G_OBJECT(wrap),
                   "size-allocate",
                   G_CALLBACK(suil_x11_on_size_allocate),
                   NULL);

  g_signal_connect(
    G_OBJECT(wrap), "map-event", G_CALLBACK(suil_x11_on_map_event), NULL);

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
  (void)host;
  (void)host_type_uri;
  (void)ui_type_uri;

  SuilWrapper* wrapper = (SuilWrapper*)calloc(1, sizeof(SuilWrapper));
  wrapper->wrap        = wrapper_wrap;
  wrapper->free        = wrapper_free;

  SuilX11Wrapper* const wrap =
    SUIL_X11_WRAPPER(g_object_new(SUIL_TYPE_X11_WRAPPER, NULL));

  wrapper->impl             = wrap;
  wrapper->resize.handle    = wrap;
  wrapper->resize.ui_resize = wrapper_resize;

  gtk_widget_set_sensitive(GTK_WIDGET(wrap), TRUE);
  gtk_widget_set_can_focus(GTK_WIDGET(wrap), TRUE);

  const intptr_t parent_id = (intptr_t)gtk_plug_get_id(wrap->plug);
  suil_add_feature(features, &n_features, LV2_UI__parent, (void*)parent_id);
  suil_add_feature(features, &n_features, LV2_UI__resize, &wrapper->resize);
  suil_add_feature(features, &n_features, LV2_UI__idleInterface, NULL);

  // Scan for URID map and options
  LV2_URID_Map*       map     = NULL;
  LV2_Options_Option* options = NULL;
  for (LV2_Feature** f = *features; *f && (!map || !options); ++f) {
    if (!strcmp((*f)->URI, LV2_OPTIONS__options)) {
      options = (LV2_Options_Option*)(*f)->data;
    } else if (!strcmp((*f)->URI, LV2_URID__map)) {
      map = (LV2_URID_Map*)(*f)->data;
    }
  }

  if (map && options) {
    // Set UI update rate if given
    LV2_URID ui_updateRate = map->map(map->handle, LV2_UI__updateRate);
    for (LV2_Options_Option* o = options; o->key; ++o) {
      if (o->key == ui_updateRate) {
        wrap->idle_ms = (guint)(1000.0f / *(const float*)o->value);
        break;
      }
    }
  }

  return wrapper;
}
