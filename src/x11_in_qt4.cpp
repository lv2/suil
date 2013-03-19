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

#include <QTimerEvent>
#include <QX11EmbedContainer>
#undef signals

#include "./suil_config.h"
#include "./suil_internal.h"

#ifndef HAVE_NEW_LV2
typedef struct _LV2UI_Idle_Interface LV2UI_Idle_Interface;
#endif

extern "C" {

class SuilQX11Container : public QX11EmbedContainer
{
public:
	SuilQX11Container(SuilInstance*               instance,
	                  const LV2UI_Idle_Interface* idle_iface)
		: QX11EmbedContainer()
		, _instance(instance)
		, _idle_iface(idle_iface)
		, _ui_timer(0)
	{}

#ifdef HAVE_NEW_LV2
	void showEvent(QShowEvent* event) {
		if (_idle_iface && _ui_timer == 0) {
			_ui_timer = this->startTimer(30);
		}
		QX11EmbedContainer::showEvent(event);
	}

	void timerEvent(QTimerEvent* event) {
		if (event->timerId() == _ui_timer && _idle_iface) {
			_idle_iface->idle(_instance->handle);
		}
		QX11EmbedContainer::timerEvent(event);
	}
#endif

	SuilInstance*               _instance;
	const LV2UI_Idle_Interface* _idle_iface;
	int                         _ui_timer;
};

static int
wrapper_wrap(SuilWrapper*  wrapper,
             SuilInstance* instance)
{
	const LV2UI_Idle_Interface* idle_iface = NULL;
#ifdef HAVE_NEW_LV2
	idle_iface = (const LV2UI_Idle_Interface*)suil_instance_extension_data(
		instance, LV2_UI__idleInterface);
#endif

	QX11EmbedWidget* const   ew   = (QX11EmbedWidget*)wrapper->impl;
	SuilQX11Container* const wrap = new SuilQX11Container(instance, idle_iface);

	ew->embedInto(wrap->winId());

	instance->host_widget = wrap;

	return 0;
}

static int
wrapper_resize(LV2UI_Feature_Handle handle, int width, int height)
{
	QX11EmbedWidget* const ew = (QX11EmbedWidget*)handle;
	ew->resize(width, height);
	return 0;
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
	wrapper->free = NULL;

	QX11EmbedWidget* const ew = new QX11EmbedWidget();

	wrapper->impl             = ew;
	wrapper->resize.handle    = ew;
	wrapper->resize.ui_resize = wrapper_resize;

	suil_add_feature(features, &n_features, LV2_UI__parent,
	                 (void*)(intptr_t)ew->winId());

	suil_add_feature(features, &n_features, LV2_UI__resize,
	                 &wrapper->resize);

#ifdef HAVE_NEW_LV2
	suil_add_feature(features, &n_features, LV2_UI__idleInterface, NULL);
#endif

	return wrapper;
}

}  // extern "C"
