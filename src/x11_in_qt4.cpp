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

#include <QX11EmbedContainer>
#undef signals

#include "./suil_config.h"
#include "./suil_internal.h"

extern "C" {

static int
wrapper_wrap(SuilWrapper*  wrapper,
             SuilInstance* instance)
{
	QX11EmbedWidget* const    ew   = (QX11EmbedWidget*)wrapper->impl;
	QX11EmbedContainer* const wrap = new QX11EmbedContainer();

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

SUIL_API
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

	suil_add_feature(features, n_features++, LV2_UI__parent,
	                 (void*)(intptr_t)ew->winId());

	suil_add_feature(features, n_features++, LV2_UI__resize,
	                 &wrapper->resize);

	return wrapper;
}

}  // extern "C"
