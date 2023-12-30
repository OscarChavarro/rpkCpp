#ifndef __BREP_EDGE__
#define __BREP_EDGE__

#include "BREP/BREP_WING.h"

class BREP_EDGE {
  public:
    void *client_data;
    BREP_WING wing[2]; // two wings
};

/* release all storage associated with an edge if not used anymore in
 * any contour (the given edge must already be disconnected from its contours.) */
extern void BrepDestroyEdge(BREP_EDGE *edge);

#endif
