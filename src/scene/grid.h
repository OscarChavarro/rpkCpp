/*
Global_Raytracer_activeRaytracer acceleration using a uniform grid
*/

#ifndef _GRID_H_
#define _GRID_H_

#include "common/bounds.h"
#include "skin/patch.h"
#include "skin/geom.h"

class GRIDITEM {
  public:
    void *ptr;        /* PATCH or GEOM pointer */
    unsigned flags;    /* patch or geom? last ray id, ... */
};

class GRIDITEMLIST {    /* same layout as LIST in List.h */
  public:
    GRIDITEM *elem;
    GRIDITEMLIST *next;
};

class GRID {
  public:
    short xsize, ysize, zsize;    /* dimensions */
    Vector3D voxsize;
    GRIDITEMLIST **items;        /* 3D array of item lists */
    BOUNDINGBOX bounds;        /* bounding box */
    void **gridItemPool;
};

extern GRID *CreateGrid(GEOM *geom);

extern void DestroyGrid(GRID *grid);

extern HITREC *
GridIntersect(GRID *grid, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

/* traces a ray through a voxel grid. Returns a list of all intersections. */
extern HITLIST *AllGridIntersections(HITLIST *hits, GRID *grid, Ray *ray, float mindist, float maxdist, int hitflags);

#endif
