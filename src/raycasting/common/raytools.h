/**
Some utility routines for ray intersections and for statistics
*/

#ifndef __RAY_TOOLS__
#define __RAY_TOOLS__

#include "common/Ray.h"
#include "material/bsdf.h"
#include "skin/Patch.h"
#include "raycasting/common/pathnode.h"

extern RayHit *findRayIntersection(Ray *ray, Patch *patch, BSDF *currentBsdf, RayHit *hitStore);
extern bool pathNodesVisible(CPathNode *node1, CPathNode *node2);
extern bool eyeNodeVisible(CPathNode *eyeNode, CPathNode *node, float *pix_x, float *pix_y);

extern bool GLOBAL_rayCasting_interruptRaytracing;

#endif
