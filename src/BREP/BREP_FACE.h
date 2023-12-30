#ifndef __BREP_FACE__
#define __BREP_FACE__

#include "BREP/BREP_CONTOUR.h"

class BREP_SOLID;

/**
A face is a bounded open subset of a plane, bounded by one or more
contours. One contour defines the outside of the face. Other contours
describe its cavities. The contours describing cavities are called
rings. The orientation of the outer contour must be counterclockwise as
seen from the direction of the normal to the containing plane. Rings
must be oriented clockwise. The cross product of a contour edge direction
and the plane normal is always a vector that points to the outside of
a face. Or in other words, the inside of a face is always on the left side
of a contour. The geometric data supplied by the user must conform with
this rule. The rings, if any, are pointed to by the outer contour.
When building a face, the first contour being specified must be the
outer contour (at least if your application relies on this
interpretation).
*/
class BREP_FACE {
  public:
    // faces form a doubly linked ring
    BREP_FACE *prev;
    BREP_FACE *next;
    BREP_CONTOUR *outer_contour;
    BREP_SOLID *shell; // back pointer to the containing shell
};

/* release all storage associated with a face and its contours, including
 * edges if not used in other faces as well */
extern void BrepDestroyFace(BREP_FACE *face);

#include "BREP/BREP_SOLID.h"

#endif
