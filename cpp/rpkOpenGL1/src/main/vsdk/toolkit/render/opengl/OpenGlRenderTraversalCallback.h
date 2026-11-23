#ifndef OPENGL_RENDER_TRAVERSAL_CALLBACK__
#define OPENGL_RENDER_TRAVERSAL_CALLBACK__

#include "vsdk/toolkit/render/opengl/OpenGLCallbacks.h"

class OpenGlRenderTraversalCallback {
  public:
    OpenGlRenderPatchCallback callbackWithoutData;
    OpenGlRenderPatchCallbackWithData callbackWithData;
    const FaceColorTable *callbackData;

    OpenGlRenderTraversalCallback(OpenGlRenderPatchCallback callback,
                                  OpenGlRenderPatchCallbackWithData defaultCallback,
                                  const FaceColorTable *colors):
        callbackWithoutData(callback), callbackWithData(defaultCallback), callbackData(colors) {}
};

#endif
