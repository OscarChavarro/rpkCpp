#ifndef __OPEN_GL_CALLBACKS__
#define __OPEN_GL_CALLBACKS__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"

using OpenGlRenderPatchCallback = void (*)(const Patch *, const Camera *, const RendererConfiguration *);
class FaceColorTable;
using OpenGlRenderPatchCallbackWithData = void (*)(const Patch *, const Camera *, const RendererConfiguration *, const FaceColorTable *);

#endif
