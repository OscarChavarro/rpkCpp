#ifndef POTENTIAL__
#define POTENTIAL__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"

class Potential {
  private:
    static void softGetPatchPointers(const SglContext *sgl, const java::ArrayList<Patch *> *scenePatches);
    static void softUpdateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions);

  public:
    static void updateDirectPotential(const Scene *scene, const RendererConfiguration *renderOptions);
    static void updateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions);
};

#endif
