/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef SOFT_IDS__
#define SOFT_IDS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/render/FaceVisibilityBuffer.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class SoftIds {
  private:
    static void drawPatch(const Patch &patch, const Camera &camera,
                          const RendererConfiguration &renderOptions, FaceVisibilityBuffer &target);

  public:
    /** Renders into a cleared target with the camera's dimensions and full viewport. */
    static void renderVisibleFaces(const Scene &scene, const RendererConfiguration *renderOptions, FaceVisibilityBuffer &target);
    static unsigned long *softRenderIds(long *x, long *y, const Scene *scene, const RendererConfiguration *renderOptions);
    static void softRenderPixels(int width, int height, const ColorRgbMutable *rgb, const ToneMappingContext &toneMapOptions);
};

#endif
