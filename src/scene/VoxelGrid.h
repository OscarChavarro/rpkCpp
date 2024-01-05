/*
Global_Raytracer_activeRaytracer acceleration using a uniform grid
*/

#ifndef _GRID_H_
#define _GRID_H_

#include "java/util/ArrayList.h"
#include "skin/bounds.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"
#include "scene/VoxelData.h"

class GRIDITEMLIST {    /* same layout as LIST in List.h */
  public:
    VoxelData *elem;
    GRIDITEMLIST *next;
};

class VoxelGrid {
  public:
    short xSize;
    short ySize;
    short zSize;
    Vector3D voxelSize;
    GRIDITEMLIST **volumeListsOfItems;        /* 3D array of item lists */
    void **gridItemPool;
    BOUNDINGBOX boundingBox;        /* bounding box */
};

extern VoxelGrid *CreateGrid(Geometry *geom);
extern void DestroyGrid(VoxelGrid *grid);
extern HITREC *
GridIntersect(VoxelGrid *grid, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

/* traces a ray through a voxel grid. Returns a list of all intersections. */
extern HITLIST *AllGridIntersections(HITLIST *hits, VoxelGrid *grid, Ray *ray, float mindist, float maxdist, int hitflags);

#endif
