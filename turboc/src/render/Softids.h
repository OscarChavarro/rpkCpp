/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef __SOFT_IDS__
#define __SOFT_IDS__

#include "java/util/ArrayList.h"
#include "material/RendererConfiguration.h"
#include "environment/geometry/elements/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "render/sgl/SglContext.h"
#include "tonemap/ToneMappingContext.h"

class SoftIds {
  public:
    static SglContext *setupSoftFrameBuffer(const Camera *camera);
    static void softRenderPatch(
        const Patch *patch,
        const Camera *camera,
        const RenderOptions *renderOptions,
        SglContext *sglContext);
    static void softRenderPatches(const Scene *scene, const RenderOptions *renderOptions, SglContext *sglContext);
    static unsigned long *softRenderIds(long *x, long *y, const Scene *scene, const RenderOptions *renderOptions);
    static void softRenderPixels(int width, int height, const ColorRgb *rgb, const ToneMappingContext &toneMapOptions);
};

#endif
