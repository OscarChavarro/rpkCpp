/**
Generate and trace a local line
*/

#ifndef __LOCAL_LINE__
#define __LOCAL_LINE__

#include "common/linealAlgebra/Ray.h"

class CoordinateSystem;
class Patch;
class RayHit;
class VoxelGrid;

class Localline final {
  public:
    static Ray mcrGenerateLocalLine(const Patch *patch, const double *xi);
    static RayHit *mcrShootRay(const VoxelGrid *sceneWorldVoxelGrid, Patch *patch, Ray *ray, RayHit *hitStore);

  private:
    static void patchCoordSys(const Patch *patch, CoordinateSystem *coord);
    static void someFeedback();
};

#endif
