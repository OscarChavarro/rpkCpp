#ifndef __RENDER_HOOK__
#define __RENDER_HOOK__

using RenderHookFunction = void (*)(void *data);

class RenderHook {
  public:
    RenderHookFunction function;
    void *data;
};

#endif
