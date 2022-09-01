// Copyright 2017-2021 David Robillard <d@drobilla.net>
// Copyright 2017 Stefan Westerfeld <stefan@space.twc.de>
// SPDX-License-Identifier: ISC

#include "suil_internal.h"

#include "suil/suil.h"

#include <X11/Xlib.h>

SUIL_LIB_EXPORT
void
suil_host_init(void)
{
  // This must be called first for Qt5 in Gtk2 to function correctly
  XInitThreads();
}
