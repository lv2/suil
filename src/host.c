// Copyright 2011-2021 David Robillard <d@drobilla.net>
// Copyright 2017 Stefan Westerfeld <stefan@space.twc.de>
// SPDX-License-Identifier: ISC

#include "dylib.h"
#include "suil_config.h"
#include "suil_internal.h"

#include "suil/suil.h"

#include <stdlib.h>

int    suil_argc = 0;
char** suil_argv = NULL;

SUIL_API SuilHost*
suil_host_new(SuilPortWriteFunc       write_func,
              SuilPortIndexFunc       index_func,
              SuilPortSubscribeFunc   subscribe_func,
              SuilPortUnsubscribeFunc unsubscribe_func)
{
  SuilHost* host = (SuilHost*)calloc(1, sizeof(struct SuilHostImpl));

  host->write_func       = write_func;
  host->index_func       = index_func;
  host->subscribe_func   = subscribe_func;
  host->unsubscribe_func = unsubscribe_func;
  host->argc             = suil_argc;
  host->argv             = suil_argv;

  return host;
}

SUIL_API void
suil_host_set_touch_func(SuilHost* host, SuilTouchFunc touch_func)
{
  host->touch_func = touch_func;
}

SUIL_API void
suil_host_free(SuilHost* host)
{
  if (host) {
    if (host->gtk_lib) {
      dylib_close(host->gtk_lib);
    }

    free(host);
  }
}

#if USE_X11
static void
suil_load_init_module(const char* module_name)
{
  void* const lib = suil_open_module(module_name);
  if (!lib) {
    return;
  }

  SuilVoidFunc init_func = suil_dlfunc(lib, "suil_host_init");
  if (init_func) {
    (*init_func)();
  } else {
    SUIL_ERRORF("Corrupt init module %s\n", module_name);
  }

  dylib_close(lib);
}
#endif

SUIL_API void
suil_init(int* argc, char*** argv, SuilArg key, ...)
{
  (void)key;

  suil_argc = argc ? *argc : 0;
  suil_argv = argv ? *argv : NULL;

#if USE_X11
  suil_load_init_module("suil_x11");
#endif
}
