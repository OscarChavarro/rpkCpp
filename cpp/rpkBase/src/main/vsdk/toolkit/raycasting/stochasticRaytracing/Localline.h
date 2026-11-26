/**
Generate and trace a local line
*/

#ifndef __LOCALLINE__
#define __LOCALLINE__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Localline final {
  public:
    static Ray mcrGenerateLocalLine(const Patch *patch, const double *xi);
    static RpkRayHit *mcrShootRay(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, const Patch *patch, Ray *ray, RpkRayHit *hitStore);

  private:
    static void patchCoordSys(const Patch *patch, CoordinateSystem *coord);
    static void someFeedback();
};

#endif
