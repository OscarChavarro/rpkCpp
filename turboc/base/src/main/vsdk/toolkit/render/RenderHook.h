#ifndef __RENDER_HOOK_PRIVATE__
#define __RENDER_HOOK_PRIVATE__

#include "vsdk/toolkit/common/VSDK.h"

typedef void (*RenderHookFunction)(void *data);

class RenderHook {
  public:
    RenderHookFunction function;
    void *data;
};

#endif
