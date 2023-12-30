#ifndef __BREP_VERTEX_OCTREE__
#define __BREP_VERTEX_OCTREE__

#include "BREP/BREP_VERTEX.h"

/* ********************** vertex octrees *************************** */
/* vertices can also be sorted and stored into an octree in order to make
 * searching for a vertex significantly faster. */
class BREP_VERTEX_OCTREE {
  public:
    BREP_VERTEX *vertex;
    BREP_VERTEX_OCTREE *child[8];
};

/* Before trying to store vertices in an octree, first a routine should
 * be speciied to compare the client data of two vertices.
 * The routine specified should return a code with the following meaning:
 *
 * 	0:  x1 <= x2 , y1 <= y2, z1 <= z2 but not all are equal
 * 	1:  x1 >  x2 , y1 <= y2, z1 <= z2
 * 	2:  x1 <= x2 , y1 >  y2, z1 <= z2
 * 	3:  x1 >  x2 , y1 >  y2, z1 <= z2
 * 	4:  x1 <= x2 , y1 <= y2, z1 >  z2
 * 	5:  x1 >  x2 , y1 <= y2, z1 >  z2
 * 	6:  x1 <= x2 , y1 >  y2, z1 >  z2
 * 	7:  x1 >  x2 , y1 >  y2, z1 >  z2
 * 	8:  x1 == x2 , y1 == y2, z1 == z2
 *
 * in other words:
 *
 *	code&1 == 1 if x1 > x2 and 0 otherwise
 *	code&2 == 1 if y1 > y2 and 0 otherwise
 *	code&4 == 1 if z1 > z2 and 0 otherwise
 *	code&8 == 1 if x1 == x2 and y1 == y2 and z1 == z2
 *
 * BrepSetVertexCompareRoutine() returns the previously installed compare
 * routine so it can be restored when necessary. */
typedef int (*BREP_COMPARE_FUNC)(void *, void *);

extern BREP_COMPARE_FUNC BrepSetVertexCompareRoutine(BREP_COMPARE_FUNC routine);

/* Set a routine to compare only the the location of two BREP_VERTEXes. There
 * may be multiple vertices at the same location, e.g. having a different
 * normal and/or name. These vertices are considered different vertices
 * by the vertex compare routine which is set with BrepSetVertexCompareRoutine(),
 * but they are considered the same vertices by the routine which is set with
 * BrepSetVertexCompareLocationRoutine(). The previously installed compare
 * routine is returned, so it can be restored when necessary. This compare
 * routine is used by BrepFindVertexAtLocation(), BrepIterateVerticesAtLocation(),
 * BrepIterateWingsBetweenLocations() ... */
extern BREP_COMPARE_FUNC BrepSetVertexCompareLocationRoutine(BREP_COMPARE_FUNC routine);

#endif

