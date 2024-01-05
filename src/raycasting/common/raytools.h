/**
Some utility routines for ray intersections and for statistics
*/

#ifndef _RAYTOOLS_H_
#define _RAYTOOLS_H_

#include "common/Ray.h"
#include "material/bsdf.h"
#include "skin/Patch.h"
#include "raycasting/common/pathnode.h"

extern HITREC *FindRayIntersection(Ray *ray, PATCH *patch, BSDF *currentBsdf, HITREC *hitstore);
extern bool PathNodesVisible(CPathNode *node1, CPathNode *node2);
extern bool EyeNodeVisible(CPathNode *eye, CPathNode *node, float *pix_x, float *pix_y);

extern bool interrupt_raytracing;

#endif
