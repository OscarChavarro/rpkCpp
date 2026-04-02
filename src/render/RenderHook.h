#ifndef __RENDER_HOOK_PRIVATE__
#define __RENDER_HOOK_PRIVATE__

#include "render/RenderHookList.h"

class RenderHook {
  public:
    RenderHookFunction function;
    void *data;
};

#endif
