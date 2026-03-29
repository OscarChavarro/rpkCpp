#ifndef __RENDER_HOOK_LIST__
#define __RENDER_HOOK_LIST__

/**
Render hooks are called each time the scene is rendered.
Functions are provided to add and remove hooks.
Hooks should only depend on render.h, not on GLX or OpenGL
*/

typedef void (*RenderHookFunction)(void *data);

void renderHooks();
void removeAllRenderHooks();

#endif
