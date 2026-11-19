/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef __SOFT_IDS__
#define __SOFT_IDS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

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
