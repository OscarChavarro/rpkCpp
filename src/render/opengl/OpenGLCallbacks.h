#ifndef __OPEN_GL_CALLBACKS__
#define __OPEN_GL_CALLBACKS__

#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/Camera.h"

using OpenGlRenderPatchCallback = void (*)(const Patch *, const Camera *, const RenderOptions *);
using OpenGlRenderPatchCallbackWithData = void (*)(const Patch *, const Camera *, const RenderOptions *, void *);

#endif
