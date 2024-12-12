// Copyright 2020-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "x11_util.h"

#include <X11/X.h>
#include <X11/Xlib.h>

#include <stddef.h>

bool
suil_x11_is_valid_child(Display* const display,
                        const Window   parent,
                        const Window   child)
{
  Window   root        = 0U;
  Window   grandparent = 0U;
  Window*  children    = NULL;
  unsigned n_children  = 0U;

  XQueryTree(display, parent, &root, &grandparent, &children, &n_children);
  if (children) {
    for (unsigned i = 0U; i < n_children; ++i) {
      if (children[i] == child) {
        XFree(children);
        return true;
      }
    }

    XFree(children);
  }

  return false;
}

Window
suil_x11_get_parent(Display* display, Window child)
{
  Window   root     = 0U;
  Window   parent   = 0U;
  Window*  children = NULL;
  unsigned count    = 0U;

  if (child) {
    if (XQueryTree(display, child, &root, &parent, &children, &count)) {
      if (children) {
        XFree(children);
      }
    }
  }

  return (parent == root) ? 0 : parent;
}
