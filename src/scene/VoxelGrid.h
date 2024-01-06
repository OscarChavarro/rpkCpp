/**
Global_Raytracer_activeRaytracer acceleration using a uniform grid
*/

#ifndef _GRID_H_
#define _GRID_H_

#include "common/mymath.h"
#include "java/util/ArrayList.h"
#include "skin/bounds.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"
#include "scene/VoxelData.h"

#define ISPATCH 0x10000000
#define ISGEOM  0x20000000
#define ISGRID  0x40000000
#define RAYCOUNT_MASK 0x0fffffff

#define LastRayId(griditem) (griditem->flags & RAYCOUNT_MASK)
#define UpdateRayId(griditem, id) griditem->flags = (griditem->flags & ~RAYCOUNT_MASK) | (id & RAYCOUNT_MASK)
#define IsPatch(griditem) (griditem->flags & ISPATCH)
#define IsGeom(griditem)  (griditem->flags & ISGEOM)
#define IsGrid(griditem)  (griditem->flags & ISGRID)

#define CELLADDR(grid, a, b, c) ((a * grid->ySize + b) * grid->zSize + c)

#define x2voxel(px, g)    ((g->voxelSize.x<EPSILON) ? 0 : ((px) - g->boundingBox[MIN_X]) / g->voxelSize.x)
#define y2voxel(py, g)    ((g->voxelSize.y<EPSILON) ? 0 : ((py) - g->boundingBox[MIN_Y]) / g->voxelSize.y)
#define z2voxel(pz, g)    ((g->voxelSize.z<EPSILON) ? 0 : ((pz) - g->boundingBox[MIN_Z]) / g->voxelSize.z)

#define voxel2x(px, g)    ((px) * g->voxelSize.x + g->boundingBox[MIN_X])
#define voxel2y(py, g)    ((py) * g->voxelSize.y + g->boundingBox[MIN_Y])
#define voxel2z(pz, g)    ((pz) * g->voxelSize.z + g->boundingBox[MIN_Z])

#include "common/dataStructures/List.h"

#define GridItemListCreate    (GRIDITEMLIST *)ListCreate
#define GridItemListAdd(griditemlist, griditem)    \
        (GRIDITEMLIST *)ListAdd((LIST *)griditemlist, (void *)griditem)
#define GridItemListDestroy(griditemlist) \
        ListDestroy((LIST *)griditemlist)
#define ForAllGridItems(item, griditemlist) ForAllInList(VoxelData, item, griditemlist)

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
    GRIDITEMLIST **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    BOUNDINGBOX boundingBox;
};

extern VoxelGrid *createGrid(Geometry *geom);
extern void destroyGrid(VoxelGrid *grid);
extern HITREC *
GridIntersect(VoxelGrid *grid, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

extern HITLIST *allGridIntersections(HITLIST *hits, VoxelGrid *grid, Ray *ray, float mindist, float maxdist, int hitflags);

#endif
