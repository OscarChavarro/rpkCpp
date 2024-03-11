#ifndef __RENDER_HOOK_PRIVATE__
#define __RENDER_HOOK_PRIVATE__

#include "render/renderhook.h"

class RenderHook {
public:
    RenderHookFunction func;
    void *data;
};

#endif
