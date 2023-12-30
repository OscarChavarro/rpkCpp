#ifndef __BREP_WING__
#define __BREP_WING__

#include "BREP/BREP_VERTEX.h"

class BREP_CONTOUR;
class BREP_EDGE;

/**
An edge is a unique connection between two vertices. There can be no more
than one edge between two given vertices in a valid model. An edge can be
used in two contours. The use of an edge in a contour is called a
wing. A wing contains the starting vertex of the edge in the contour. The
other vertex is the starting vertex in the other wing, since the edge should
be used in opposite sense by the other contour sharing the edge.
The invariant is: wing->vertex == OtherWingInEdge(wing->prev)->vertex:
the starting point in a wing is the endpoint of the edge specified in
wing->prev.
The data structure contains two more pointers than a winged edge data
structure, but the code to operate on it is significantly simpler, cfr.
[Weiler 1985]. The data structure is more compact than in [Glassner 1991].
*/
class BREP_WING { // A.k.a. half-edge
  public:
    BREP_VERTEX *vertex; // start vertex
    // ClockWise and CounterClockWise next wing within outer contours
    BREP_WING *prev;
    BREP_WING *next;
    BREP_EDGE *edge; // pointer to the containing edge
    BREP_CONTOUR *contour; // back pointer to the containing contour
};

extern void BrepDisconnectWingFromContour(BREP_WING *wing);
extern void BrepDestroyWing(BREP_WING *wing);

#include "BREP/BREP_EDGE.h"
#include "BREP/BREP_CONTOUR.h"

#endif
