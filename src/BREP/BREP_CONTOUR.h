#ifndef __BREP_CONTOUR__
#define __BREP_CONTOUR__

#include "BREP/BREP_EDGE.h"

class BREP_FACE;

/**
A contour represents a single closed planar polygonal curve (a loop).
Self-intersecting contours are not allowed. Different contours may only
intersect at common vertices or edges if at all. Here, we provide pointers
to WINGs instead of EDGEs in order to have implicit information about
the orientation of an edge as used in a contour. This facilitates
iterating over the edges or vertices in a contour in a consistent manner.
When building a contour, the edges must be specified in the right order:
the inside of the face is always to the left of an edge. In a two-manifold,
outer contours do not touch inner contours (they don't intersect
and do not share vertices or edges). However, a vertex may be
used several times in a contour and two outer or inner contours
can share an edge, but only in opposite direction. If you need multiple
pairs of contours sharing the same vertex, you probably want
topologically different vertices at the same place in 3D space. Same if
you believe you need inner and outer contours sharing a vertex.
*/
class BREP_CONTOUR {
  public:
    // Contours form a doubly linked ring in order to facilitate the splitting
    // and merging of contours. There is one ring per face, containing the
    // outer contour as well as the rings
    BREP_CONTOUR *prev;
    BREP_CONTOUR *next;
    BREP_WING *wings;
    BREP_FACE *face; // back pointer to the containing face
};

/* disconnects a contour from its containing face */
extern void BrepDisconnectContourFromFace(BREP_CONTOUR *contour);

/* release all storage associated with a contour and its wing/edges
 * if not used in other contours as well */
extern void
BrepDestroyContour(BREP_CONTOUR *contour);

#include "BREP/BREP_FACE.h"

#endif
