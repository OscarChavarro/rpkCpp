#ifndef RENDER_HOOK_LIST__
#define RENDER_HOOK_LIST__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/render/RenderHook.h"

/**
Render hooks are called each time the scene is rendered.
Functions are provided to add and remove hooks.
Hooks should only depend on render.h, not on GLX or OpenGL
*/

class RenderHookList {
  private:
    static java::ArrayList<RenderHook *> *renderHookList;

  public:
    static void renderHooks();
    static void removeAllRenderHooks();
};

#endif
