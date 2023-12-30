/* vertex_octree.h: Boundary Representation vertices organised in an
 *		    octree. For fast vertex lookup. */

#ifndef _BREP_VERTEX_OCTREE_H_
#define _BREP_VERTEX_OCTREE_H_

#include "BREP/BREP_SOLID.h"
#include "common/dataStructures/Octree.h"

#define BrepVertexOctreeCreate()    ((BREP_VERTEX_OCTREE *)OctreeCreate())

#define BrepVertexOctreeAdd(poctree, pbrep_vertex)     \
        (BREP_VERTEX_OCTREE *)OctreeAddWithDuplicates((OCTREE *)poctree, (void *)pbrep_vertex, (int (*)(void *, void *))BrepVertexCompare)

#define BrepVertexOctreeFind(poctree, pbrep_vertex)     \
        (BREP_VERTEX *)OctreeFind((OCTREE *)poctree, (void *)pbrep_vertex, (int (*)(void *, void *))BrepVertexCompare)

#define BrepVertexOctreeDestroy(poctree) \
        OctreeDestroy((OCTREE *)poctree)

#define BrepVertexOctreeIterate(poctree, func) \
        OctreeIterate((OCTREE *)poctree, (void (*)(void *))func)

/* compares two vertices. Calls a user installed routine to compare the
 * client data of two vertices. The routine to be used is specified with
 * BrepSetVertexCompareRoutine() in brep.h */
extern int BrepVertexCompare(BREP_VERTEX *v1, BREP_VERTEX *v2);

#endif
