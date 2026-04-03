#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "render/sgl/SglContext.h"

class Potential {
  private:
    static void softGetPatchPointers(const SglContext *sgl, const java::ArrayList<Patch *> *scenePatches);
    static void softUpdateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions);

  public:
    static void updateDirectPotential(const Scene *scene, const RenderOptions *renderOptions);
    static void updateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions);
};

#endif
