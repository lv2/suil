// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "suil_internal.h"
#include "warnings.h"

#include <lv2/options/options.h>
#include <lv2/urid/urid.h>

SUIL_DISABLE_GTK_WARNINGS
#include <gdk/gdkwin32.h>
#include <gtk/gtk.h>
SUIL_RESTORE_WARNINGS

#ifndef WM_MOUSEWHEEL
#  define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#  define WM_MOUSEHWHEEL 0x020E
#endif

#include <string.h>

extern "C" {

#define SUIL_TYPE_WIN_WRAPPER (suil_win_wrapper_get_type())
#define SUIL_WIN_WRAPPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), SUIL_TYPE_WIN_WRAPPER, SuilWinWrapper))

struct _SuilWinWrapper {
  GtkDrawingArea              area;
  SuilWrapper*                wrapper;
  SuilInstance*               instance;
  GdkWindow*                  flt_win;
  const LV2UI_Idle_Interface* idle_iface;
  guint                       idle_id;
  guint                       idle_ms;
};

struct _SuilWinWrapperClass {
  GtkDrawingAreaClass parent_class;
};

using SuilWinWrapper      = _SuilWinWrapper;
using SuilWinWrapperClass = _SuilWinWrapperClass;

GType
suil_win_wrapper_get_type(void); // Accessor for SUIL_TYPE_WIN_WRAPPER

G_DEFINE_TYPE(SuilWinWrapper, suil_win_wrapper, GTK_TYPE_DRAWING_AREA)

static void
suil_win_wrapper_finalize(GObject* gobject)
{
  SuilWinWrapper* const self = SUIL_WIN_WRAPPER(gobject);

  self->wrapper->impl = nullptr;
  self->instance      = nullptr;

  G_OBJECT_CLASS(suil_win_wrapper_parent_class)->finalize(gobject);
}

static void
suil_win_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
  SuilWinWrapper* const self = SUIL_WIN_WRAPPER(widget);
  g_return_if_fail(self != nullptr);

  widget->allocation = *allocation;
  if (gtk_widget_get_realized(widget)) {
    gdk_window_move_resize(widget->window,
                           allocation->x,
                           allocation->y,
                           allocation->width,
                           allocation->height);

    RECT wr = {0, 0, (long)allocation->width, (long)allocation->height};
    AdjustWindowRectEx(&wr, WS_CHILD, FALSE, WS_EX_TOPMOST);

    SetWindowPos((HWND)self->instance->ui_widget,
                 HWND_NOTOPMOST,
                 0,
                 0,
                 wr.right - wr.left,
                 wr.bottom - wr.top,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER |
                   SWP_NOZORDER);
    UpdateWindow((HWND)self->instance->ui_widget);
    PostMessage((HWND)self->instance->ui_widget, WM_PAINT, 0, 0);
  }
}

static void
suil_win_wrapper_class_init(SuilWinWrapperClass* klass)
{
  GObjectClass* const   gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass* const widget_class  = (GtkWidgetClass*)(klass);

  widget_class->size_allocate = suil_win_size_allocate;
  gobject_class->finalize     = suil_win_wrapper_finalize;
}

static void
suil_win_wrapper_init(SuilWinWrapper* self)
{
  self->instance   = nullptr;
  self->flt_win    = nullptr;
  self->idle_iface = nullptr;
  self->idle_ms    = 1000 / 30; // 30 Hz default
}

static gboolean
suil_win_wrapper_idle(void* data)
{
  SuilWinWrapper* const wrap = SUIL_WIN_WRAPPER(data);
  wrap->idle_iface->idle(wrap->instance->handle);
  return TRUE; // Continue calling
}

static int
wrapper_resize(LV2UI_Feature_Handle handle, int width, int height)
{
  gtk_drawing_area_size(GTK_DRAWING_AREA(handle), width, height);
  return 0;
}

static int
wrapper_wrap(SuilWrapper* wrapper, SuilInstance* instance)
{
  SuilWinWrapper* const wrap = SUIL_WIN_WRAPPER(wrapper->impl);

  instance->host_widget = GTK_WIDGET(wrap);
  wrap->wrapper         = wrapper;
  wrap->instance        = instance;

  const LV2UI_Idle_Interface* idle_iface = nullptr;
  if (instance->descriptor->extension_data) {
    idle_iface =
      (const LV2UI_Idle_Interface*)instance->descriptor->extension_data(
        LV2_UI__idleInterface);
  }
  if (idle_iface) {
    wrap->idle_iface = idle_iface;
    wrap->idle_id = g_timeout_add(wrap->idle_ms, suil_win_wrapper_idle, wrap);
  }

  return 0;
}

static GdkFilterReturn
event_filter(GdkXEvent* xevent, GdkEvent* event, gpointer data)
{
  SuilWinWrapper* wrap = (SuilWinWrapper*)data;
  MSG*            msg  = (MSG*)xevent;
  if (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP) {
    // Forward keyboard events to UI window
    PostMessage(
      (HWND)wrap->instance->ui_widget, msg->message, msg->wParam, msg->lParam);
    return GDK_FILTER_REMOVE;
  }

  if (msg->message == WM_MOUSEWHEEL || msg->message == WM_MOUSEHWHEEL) {
    PostMessage(
      (HWND)wrap->instance->ui_widget, msg->message, msg->wParam, msg->lParam);
    return GDK_FILTER_REMOVE;
  }

  return GDK_FILTER_CONTINUE;
}

static void
wrapper_free(SuilWrapper* wrapper)
{
  if (wrapper->impl) {
    SuilWinWrapper* const wrap = SUIL_WIN_WRAPPER(wrapper->impl);
    if (wrap->idle_id) {
      g_source_remove(wrap->idle_id);
      wrap->idle_id = 0;
    }

    gdk_window_remove_filter(wrap->flt_win, event_filter, wrapper->impl);
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
  GtkWidget* parent = nullptr;
  for (unsigned i = 0; i < n_features; ++i) {
    if (!strcmp((*features)[i]->URI, LV2_UI__parent)) {
      parent = (GtkWidget*)(*features)[i]->data;
    }
  }

  if (!GTK_CONTAINER(parent)) {
    SUIL_ERRORF("No GtkContainer parent given for %s UI\n", ui_type_uri);
    return nullptr;
  }

  SuilWrapper* wrapper = (SuilWrapper*)calloc(1, sizeof(SuilWrapper));
  wrapper->wrap        = wrapper_wrap;
  wrapper->free        = wrapper_free;

  SuilWinWrapper* const wrap =
    SUIL_WIN_WRAPPER(g_object_new(SUIL_TYPE_WIN_WRAPPER, nullptr));

  wrap->wrapper = nullptr;

  wrapper->impl             = wrap;
  wrapper->resize.handle    = wrap;
  wrapper->resize.ui_resize = wrapper_resize;

  gtk_container_add(GTK_CONTAINER(parent), GTK_WIDGET(wrap));
  gtk_widget_set_can_focus(GTK_WIDGET(wrap), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(wrap), TRUE);
  gtk_widget_realize(GTK_WIDGET(wrap));

  GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(wrap));

  wrap->flt_win = gtk_widget_get_window(parent);
  gdk_window_add_filter(wrap->flt_win, event_filter, wrap);

  HWND parent_window = (HWND)GDK_WINDOW_HWND(window);
  suil_add_feature(features, &n_features, LV2_UI__parent, parent_window);
  suil_add_feature(features, &n_features, LV2_UI__resize, &wrapper->resize);
  suil_add_feature(features, &n_features, LV2_UI__idleInterface, nullptr);

  // Scan for URID map and options
  LV2_URID_Map*       map     = nullptr;
  LV2_Options_Option* options = nullptr;
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
        wrap->idle_ms = 1000.0f / *(const float*)o->value;
        break;
      }
    }
  }

  return wrapper;
}

} // extern "C"
