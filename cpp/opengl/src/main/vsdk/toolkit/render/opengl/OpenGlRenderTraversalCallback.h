#ifndef OPENGL_RENDER_TRAVERSAL_CALLBACK__
#define OPENGL_RENDER_TRAVERSAL_CALLBACK__

#include "vsdk/toolkit/render/opengl/OpenGLCallbacks.h"

class OpenGlRenderTraversalCallback {
  public:
    OpenGlRenderPatchCallback callbackWithoutData;
    OpenGlRenderPatchCallbackWithData callbackWithData;
    void *callbackData;
};

#endif
