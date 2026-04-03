#ifndef __RENDER_HOOK_PRIVATE__
#define __RENDER_HOOK_PRIVATE__

using RenderHookFunction = void (*)(void *data);

class RenderHook {
  public:
    RenderHookFunction function;
    void *data;
};

#endif
