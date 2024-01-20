#include <cstdlib>

#include "BREP/BREP_SOLID.h"
#include "BREP/brep_log.h"

/**
Looks whether the wing is referenced in the wing ring of its
starting vertex. Returns a pointer to the DoubleLinkedListOfWings element if
it is found and nullptr if not
*/
DoubleLinkedListOfWings *
brepFindWingLeavingVertex(BREP_WING *wing) {
    BREP_VERTEX *vertex = wing->vertex;
    DoubleLinkedListOfWings *ringel, *next;

    if ( vertex->wingRing ) {
        next = vertex->wingRing;
        do {
            ringel = next;
            next = ringel->next;
            if ( ringel->wing == wing ) {
                return ringel;
            }
        } while ( next && next != vertex->wingRing );
    }

    return (DoubleLinkedListOfWings *) nullptr;
}

/**
Disconnect the wing ring element from the wing ring it belongs to
*/
static void
brepDisconnectWingRingFromVertex(DoubleLinkedListOfWings *ringel) {
    BREP_VERTEX *vertex = ringel->wing->vertex;

    if ( vertex->wingRing == ringel ) {
        if ( ringel->next == ringel ) {        /* it's the only wing leaving the
					 * vertex */
            vertex->wingRing = (DoubleLinkedListOfWings *) nullptr;
        } else {
            vertex->wingRing = ringel->next;
        }
    }

    ringel->next->prev = ringel->prev;
    ringel->prev->next = ringel->next;
}

/**
Disconnect the wing from its starting vertex. The wing must be properly connected to
a contour
*/
void
brepDisconnectWingFromVertex(BREP_WING *wing) {
    DoubleLinkedListOfWings *ringElements;

    // Find the wing in the wing ring
    ringElements = brepFindWingLeavingVertex(wing);
    if ( !ringElements ) {
        brepError(wing->edge->client_data, "brepDisconnectWingFromVertex", "wing not connected to the vertex");
        return;
    }

    // Disconnect the wing ring element from the vertex
    brepDisconnectWingRingFromVertex(ringElements);

    // Dispose the wing ring element
    free(ringElements);
}
