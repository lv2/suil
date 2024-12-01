// Copyright 2020-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef SUIL_X11_UTIL
#define SUIL_X11_UTIL

#include <X11/X.h>
#include <X11/Xlib.h>

#include <stdbool.h>

/// Return whether `child` can be found in the subtree under `parent`
bool
suil_x11_is_valid_child(Display* display, Window parent, Window child);

/// Return the non-root parent window of `child` if it has one, or zero
Window
suil_x11_get_parent(Display* display, Window child);

#endif // SUIL_X11_UTIL
