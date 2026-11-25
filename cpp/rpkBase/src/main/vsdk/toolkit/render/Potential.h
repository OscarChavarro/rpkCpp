#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/Scene.h"

class Potential {
  private:
    static void softUpdateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions);

  public:
    static void updateDirectPotential(const Scene *scene, const RendererConfiguration *renderOptions);
    static void updateDirectVisibility(const Scene *scene, const RendererConfiguration *renderOptions);
};

#endif
