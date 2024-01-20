#include <cstdlib>

#include "BREP/BREP_EDGE.h"

/**
Release all storage associated with an edge and its vertices
if not used in other edges as well. The given edge must already
be disconnected from its contours
*/
void
brepDestroyEdge(BREP_EDGE *edge) {
    // Ccheck if the edge is still being used in a contour. If
    // so, don't destroy the edge
    if ( edge->wing[0].contour || edge->wing[1].contour ) {
        return;
    }

    // Dispose of the BREP_EDGE structure itself (inverse of
    // BrepCreateEdgeWithoutVertices()
    free(edge);
}
