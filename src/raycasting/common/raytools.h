/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/Ray.h"
#include "material/bsdf.h"
#include "skin/Patch.h"
#include "raycasting/common/pathnode.h"

extern RayHit *findRayIntersection(VoxelGrid *sceneWorldVoxelGrid, Ray *ray, Patch *patch, BSDF *currentBsdf, RayHit *hitStore);
extern bool pathNodesVisible(SimpleRaytracingPathNode *node1, SimpleRaytracingPathNode *node2);
extern bool eyeNodeVisible(SimpleRaytracingPathNode *eyeNode, SimpleRaytracingPathNode *node, float *pix_x, float *pix_y);

#endif
