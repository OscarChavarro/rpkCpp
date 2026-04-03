#ifndef __OPENGL_RENDER_TRAVERSAL_CALLBACK__
#define __OPENGL_RENDER_TRAVERSAL_CALLBACK__

#include "render/opengl/OpenGLCallbacks.h"

class OpenGlRenderTraversalCallback {
  public:
    OpenGlRenderPatchCallback callbackWithoutData;
    OpenGlRenderPatchCallbackWithData callbackWithData;
    void *callbackData;
};

#endif
