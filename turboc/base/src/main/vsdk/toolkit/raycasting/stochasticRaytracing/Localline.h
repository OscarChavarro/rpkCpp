/**
Generate and trace a local line
*/

#ifndef __LOCAL_LINE__
#define __LOCAL_LINE__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHit.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Localline{ public:
    static RpkRay mcrGenerateLocalLine(const Patch *patch, const double *xi);
    static RayHit *mcrShootRay(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Patch *patch, RpkRay *ray, RayHit *hitStore);

  private:
    static void patchCoordSys(const Patch *patch, CoordinateSystem *coord);
    static void someFeedback();
};

#endif
