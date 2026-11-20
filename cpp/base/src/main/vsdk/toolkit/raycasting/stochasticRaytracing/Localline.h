/**
Generate and trace a local line
*/

#ifndef LOCAL_LINE__
#define LOCAL_LINE__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Localline final {
  public:
    static RpkRay mcrGenerateLocalLine(const Patch *patch, const double *xi);
    static RpkRayHit *mcrShootRay(const VoxelGrid *sceneWorldVoxelGrid, VoxelGridWorkspace *sceneWorldVoxelGridWorkspace, const Patch *patch, RpkRay *ray, RpkRayHit *hitStore);

  private:
    static void patchCoordSys(const Patch *patch, CoordinateSystem *coord);
    static void someFeedback();
};

#endif
