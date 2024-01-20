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
    DoubleLinkedListOfWings *wingRing; // ring of wings leaving at the vertex
};

extern void brepDisconnectWingFromVertex(BREP_WING *wing);

#include "BREP/BREP_WING.h"

#endif
