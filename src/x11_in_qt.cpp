// Copyright 2011-2022 David Robillard <d@drobilla.net>
// Copyright 2015 Rui Nuno Capela <rncbc@rncbc.org>
// SPDX-License-Identifier: ISC

#include "suil_internal.h"
#include "warnings.h"

#include "lv2/core/lv2.h"
#include "lv2/ui/ui.h"
#include "suil/suil.h"

SUIL_DISABLE_QT_WARNINGS
#include <QResizeEvent>
#include <QSize>
#include <QTimerEvent>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <QX11Info>
#else
#  include <QGuiApplication>
#endif
SUIL_RESTORE_WARNINGS

// IWYU pragma: no_include <qguiapplication_platform.h>

#include <cstdlib>

#undef signals

namespace {

inline Display*
getX11Display()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  return QX11Info::display();
#else
  return qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
#endif
}

class SuilQX11Widget : public QWidget
{
public:
  SuilQX11Widget(QWidget* parent, Qt::WindowFlags wflags)
    : QWidget(parent, wflags)
  {}

  SuilQX11Widget(const SuilQX11Widget&)            = delete;
  SuilQX11Widget& operator=(const SuilQX11Widget&) = delete;

  SuilQX11Widget(SuilQX11Widget&&)            = delete;
  SuilQX11Widget& operator=(SuilQX11Widget&&) = delete;

  ~SuilQX11Widget() override;

  void start_idle(SuilInstance*               instance,
                  const LV2UI_Idle_Interface* idle_iface)
  {
    _instance   = instance;
    _idle_iface = idle_iface;
    if (_idle_iface && _ui_timer == 0) {
      _ui_timer = this->startTimer(30, Qt::CoarseTimer);
    }
  }

  void set_window(Window window) { _window = window; }

  QSize sizeHint() const override
  {
    if (_window) {
      XWindowAttributes attrs{};
      XGetWindowAttributes(getX11Display(), _window, &attrs);
      return {attrs.width, attrs.height};
    }

    return {0, 0};
  }

  QSize minimumSizeHint() const override
  {
    if (_window) {
      XSizeHints hints{};
      long       supplied{};
      XGetWMNormalHints(getX11Display(), _window, &hints, &supplied);
      if ((hints.flags & PMinSize)) {
        return {hints.min_width, hints.min_height};
      }
    }

    return {0, 0};
  }

protected:
  void resizeEvent(QResizeEvent* event) override
  {
    QWidget::resizeEvent(event);

    if (_window) {
      XResizeWindow(getX11Display(),
                    _window,
                    static_cast<unsigned>(event->size().width()),
                    static_cast<unsigned>(event->size().height()));
    }
  }

  void timerEvent(QTimerEvent* event) override
  {
    if (event->timerId() == _ui_timer && _idle_iface) {
      _idle_iface->idle(_instance->handle);
    }

    QWidget::timerEvent(event);
  }

  void closeEvent(QCloseEvent* event) override
  {
    if (_ui_timer && _idle_iface) {
      this->killTimer(_ui_timer);
      _ui_timer = 0;
    }

    QWidget::closeEvent(event);
  }

private:
  SuilInstance*               _instance{};
  const LV2UI_Idle_Interface* _idle_iface{};
  Window                      _window{};
  int                         _ui_timer{};
};

SuilQX11Widget::~SuilQX11Widget() = default;

struct SuilX11InQt5Wrapper {
  QWidget*        host_widget;
  SuilQX11Widget* parent;
};

void
wrapper_free(SuilWrapper* wrapper)
{
  auto* impl = static_cast<SuilX11InQt5Wrapper*>(wrapper->impl);

  delete impl->host_widget;

  free(impl);
}

int
wrapper_wrap(SuilWrapper* wrapper, SuilInstance* instance)
{
  auto* const impl = static_cast<SuilX11InQt5Wrapper*>(wrapper->impl);

  SuilQX11Widget* const ew      = impl->parent;
  Display* const        display = getX11Display();
  const auto            window  = reinterpret_cast<Window>(instance->ui_widget);

  XWindowAttributes attrs{};
  XSizeHints        hints{};
  long              supplied{};
  XSync(display, False);
  XGetWindowAttributes(display, window, &attrs);
  XGetWMNormalHints(display, window, &hints, &supplied);

  impl->parent->set_window(window);

  if ((hints.flags & PBaseSize)) {
    impl->parent->setBaseSize(hints.base_width, hints.base_height);
  }

  if ((hints.flags & PMinSize)) {
    impl->parent->setMinimumSize(hints.min_width, hints.min_height);
  }

  if ((hints.flags & PMaxSize)) {
    impl->parent->setMaximumSize(hints.max_width, hints.max_height);
  }

  if (instance->descriptor->extension_data) {
    const auto* idle_iface = static_cast<const LV2UI_Idle_Interface*>(
      instance->descriptor->extension_data(LV2_UI__idleInterface));

    ew->start_idle(instance, idle_iface);
  }

  impl->host_widget     = ew;
  instance->host_widget = impl->host_widget;

  return 0;
}

int
wrapper_resize(LV2UI_Feature_Handle handle, int width, int height)
{
  auto* const ew = static_cast<QWidget*>(handle);
  ew->resize(width, height);
  return 0;
}

} // namespace

extern "C" {

SUIL_LIB_EXPORT
SuilWrapper*
suil_wrapper_new(SuilHost*,
                 const char*,
                 const char*,
                 LV2_Feature*** features,
                 unsigned       n_features)
{
  auto* const impl =
    static_cast<SuilX11InQt5Wrapper*>(calloc(1, sizeof(SuilX11InQt5Wrapper)));

  auto* wrapper = static_cast<SuilWrapper*>(malloc(sizeof(SuilWrapper)));
  wrapper->wrap = wrapper_wrap;
  wrapper->free = wrapper_free;

  auto* const ew = new SuilQX11Widget(nullptr, Qt::Window);

  impl->parent = ew;

  wrapper->impl             = impl;
  wrapper->resize.handle    = ew;
  wrapper->resize.ui_resize = wrapper_resize;

  void* parent_id = reinterpret_cast<void*>(ew->winId());
  suil_add_feature(features, &n_features, LV2_UI__parent, parent_id);
  suil_add_feature(features, &n_features, LV2_UI__resize, &wrapper->resize);
  suil_add_feature(features, &n_features, LV2_UI__idleInterface, nullptr);

  return wrapper;
}

} // extern "C"
