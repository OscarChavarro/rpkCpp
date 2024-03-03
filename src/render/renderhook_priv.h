#ifndef __RENDER_HOOK_PRIV__
#define __RENDER_HOOK_PRIV__

#include "render/renderhook.h"

class RenderHook {
public:
    RenderHookFunction func;
    void *data;
};

#endif
