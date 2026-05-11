#ifndef RENDER_HOOK_PRIVATE__
#define RENDER_HOOK_PRIVATE__

using RenderHookFunction = void (*)(void *data);

class RenderHook {
  public:
    RenderHookFunction function;
    void *data;
};

#endif
