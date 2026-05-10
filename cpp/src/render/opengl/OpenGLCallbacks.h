#ifndef __OPEN_GL_CALLBACKS__
#define __OPEN_GL_CALLBACKS__

#include "material/RendererConfiguration.h"
#include "environment/geometry/elements/Patch.h"
#include "scene/Camera.h"

using OpenGlRenderPatchCallback = void (*)(const Patch *, const Camera *, const RendererConfiguration *);
using OpenGlRenderPatchCallbackWithData = void (*)(const Patch *, const Camera *, const RendererConfiguration *, void *);

#endif
