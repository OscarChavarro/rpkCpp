/**
Generate and trace a local line
*/

#ifndef __LOCAL_LINE__
#define __LOCAL_LINE__

#include "common/Ray.h"

extern Ray mcrGenerateLocalLine(const Patch *patch, const double *xi);
extern RayHit *mcrShootRay(const VoxelGrid * sceneWorldVoxelGrid, Patch *P, Ray *ray, RayHit *hitStore);

#endif
