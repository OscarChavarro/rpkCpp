/* gridP.h: some "private" grid-related definitions, only needed in routines 
 * that manipulate voxel grids. */

#ifndef _GRIDP_H_
#define _GRIDP_H_

#include "common/mymath.h"

/* grid item flags */
#define ISPATCH 0x10000000
#define ISGEOM  0x20000000
#define ISGRID  0x40000000
#define RAYCOUNT_MASK 0x0fffffff
#define LastRayId(griditem) (griditem->flags & RAYCOUNT_MASK)
#define UpdateRayId(griditem, id) griditem->flags = (griditem->flags & ~RAYCOUNT_MASK) | (id & RAYCOUNT_MASK)
#define IsPatch(griditem) (griditem->flags & ISPATCH)
#define IsGeom(griditem)  (griditem->flags & ISGEOM)
#define IsGrid(griditem)  (griditem->flags & ISGRID)

#include "common/dataStructures/List.h"

#define GridItemListCreate    (GRIDITEMLIST *)ListCreate

#define GridItemListAdd(griditemlist, griditem)    \
        (GRIDITEMLIST *)ListAdd((LIST *)griditemlist, (void *)griditem)

#define GridItemListDestroy(griditemlist) \
        ListDestroy((LIST *)griditemlist)

#define ForAllGridItems(item, griditemlist) ForAllInList(GRIDITEM, item, griditemlist)

#define CELLADDR(grid, a, b, c) ((a * grid->ysize + b) * grid->zsize + c)

#define x2voxel(px, g)    ((g->voxsize.x<EPSILON) ? 0 : ((px) - g->bounds[MIN_X]) / g->voxsize.x)
#define y2voxel(py, g)    ((g->voxsize.y<EPSILON) ? 0 : ((py) - g->bounds[MIN_Y]) / g->voxsize.y)
#define z2voxel(pz, g)    ((g->voxsize.z<EPSILON) ? 0 : ((pz) - g->bounds[MIN_Z]) / g->voxsize.z)

#define voxel2x(px, g)    ((px) * g->voxsize.x + g->bounds[MIN_X])
#define voxel2y(py, g)    ((py) * g->voxsize.y + g->bounds[MIN_Y])
#define voxel2z(pz, g)    ((pz) * g->voxsize.z + g->bounds[MIN_Z])

#define X 0
#define Y 1
#define Z 2


#endif /*_GRIDP_H_*/
