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

SUIL_API
SuilWrapper*
suil_wrapper_new(SuilHost*                 host,
                 const char*               host_type_uri,
                 const char*               ui_type_uri,
                 const LV2_Feature* const* features)
{
	SuilWrapper* wrapper = (SuilWrapper*)malloc(sizeof(SuilWrapper));
	wrapper->wrap = wrapper_wrap; 
	wrapper->free = (SuilWrapperFreeFunc)free;

	unsigned n_features = 0;
	for (; features[n_features]; ++n_features) {}

	QX11EmbedWidget* const ew = new QX11EmbedWidget();
	wrapper->impl = ew;

	wrapper->features = (LV2_Feature**)malloc(
		sizeof(LV2_Feature) * (n_features + 1));
	memcpy(wrapper->features, features, sizeof(LV2_Feature) * n_features);

	LV2_Feature* parent_feature = (LV2_Feature*)malloc(sizeof(LV2_Feature));
	parent_feature->URI  = NS_UI "parent";
	parent_feature->data = (void*)(intptr_t)ew->winId();

	wrapper->features[n_features] = parent_feature;

	return wrapper;
}

}  // extern "C"
