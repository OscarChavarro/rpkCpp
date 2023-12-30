#ifndef __BREP_CLASSES__
#define __BREP_CLASSES__

class BREP_WING;

/**
A doubly linked ring of wings indicates in what wings a vertex is being used.
The code becomes a lot simpler and more efficient when explicitely remembering
with each vertex what wings leave at the vertex. It also allows to deal
easier with partial shells: shells not enclosing a region into space, such
as a collection of coplanar faces, which is often used in synthetic
environments.
*/
// private class inside BREP_VERTEX
class DoubleLinkedListOfWings {
  public:
    BREP_WING *wing;
    DoubleLinkedListOfWings *prev;
    DoubleLinkedListOfWings *next;
};

// A vertex is a point in 3D space
class BREP_VERTEX {
  public:
    void *client_data;
    DoubleLinkedListOfWings *wing_ring; // ring of wings leaving at the vertex
};

/* creates a new vertex. The new vertex is not installed it in a vertex 
 * octree as vertices do not have to be sorted in an octree, see below. */
extern BREP_VERTEX *BrepCreateVertex(void *client_data);

/* ************************ Modifiers ******************************* */

/* Iterator over all wings (included in a contour) connecting the two
 * given vertices. */
extern void BrepIterateWingsWithVertex(BREP_VERTEX *vertex1, BREP_VERTEX *vertex2,
                                       void (*func)(BREP_WING *));

/* Disconnect the wing from its starting vertex. The wing must be properly
 * connected to a contour. */
extern void BrepDisconnectWingFromVertex(BREP_WING *wing);

/* release all storage associated with the vertex if it is not used
 * anymore in any edge. */
extern void BrepDestroyVertex(BREP_VERTEX *vertex);

#include "BREP/BREP_WING.h"

#endif
