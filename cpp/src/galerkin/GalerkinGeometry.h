#ifndef __GALERKIN_GEOMETRY__
#define __GALERKIN_GEOMETRY__

#include "java/util/ArrayList.h"
#include "scene/Scene.h"
#include "skin/PatchSet.h"

class GalerkinGeometry final {
  public:
    static void validateSceneForGalerkin(const Scene *scene);

    static java::ArrayList<PatchSet *> *
    collectPatchSets(const java::ArrayList<Geometry *> *geometryList);

    static java::ArrayList<PatchSet *> *
    collectPatchSets(const Geometry *geometry);

  private:
    static void validateGeometry(const Geometry *geometry, const char *context);

    static void
    collectPatchSetsRecursive(
        const Geometry *geometry,
        java::ArrayList<PatchSet *> *patchSets);
};

#endif
