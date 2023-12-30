#include <cstdlib>

#include "BREP/BREP_SOLID.h"
#include "BREP/brep_log.h"

/* creates a new vertex - no edges in which the vertex is used are specified,
 * this is done when creating an edge. */
BREP_VERTEX *BrepCreateVertex(void *client_data) {
    BREP_VERTEX *vertex;

    vertex = (BREP_VERTEX*)malloc(sizeof(BREP_VERTEX));
    vertex->wing_ring = (DoubleLinkedListOfWings *) nullptr;

    vertex->client_data = client_data;
    return vertex;
}

/* looks whether the wing is referenced in the wing ring of its
 * starting vertex. Returns a pointer to the DoubleLinkedListOfWings element if
 * it is found and nullptr if not */
DoubleLinkedListOfWings *BrepFindWingLeavingVertex(BREP_WING *wing) {
    BREP_VERTEX *vertex = wing->vertex;
    DoubleLinkedListOfWings *ringel, *next;

    if ( vertex->wing_ring ) {
        next = vertex->wing_ring;
        do {
            ringel = next;
            next = ringel->next;
            if ( ringel->wing == wing ) {
                return ringel;
            }
        } while ( next && next != vertex->wing_ring );
    }

    return (DoubleLinkedListOfWings *) nullptr;
}

/* disconnect the wing ring element from the wing ring it belongs to */
static void BrepDisconnectWingRingFromVertex(DoubleLinkedListOfWings *ringel) {
    BREP_VERTEX *vertex = ringel->wing->vertex;

    if ( vertex->wing_ring == ringel ) {
        if ( ringel->next == ringel ) {        /* it's the only wing leaving the
					 * vertex */
            vertex->wing_ring = (DoubleLinkedListOfWings *) nullptr;
        } else {
            vertex->wing_ring = ringel->next;
        }
    }

    ringel->next->prev = ringel->prev;
    ringel->prev->next = ringel->next;
}

/* Disconnect the wing from its starting vertex. The wing must be properly connected to
 * a contour. */
void BrepDisconnectWingFromVertex(BREP_WING *wing) {
    DoubleLinkedListOfWings *ringel;

    /* find the wing in the wing ring */
    ringel = BrepFindWingLeavingVertex(wing);
    if ( !ringel ) {
        BrepError(wing->edge->client_data, "BrepDisconnectWingFromVertex", "wing not connected to the vertex");
        return;
    }

    /* disconnect the wing ring element from the vertex */
    BrepDisconnectWingRingFromVertex(ringel);

    /* dispose the wing ring element */
    free(ringel);
}

/* Iterator over all wings (included in a contour) connecting the two
 * given vertices */
void
BrepIterateWingsWithVertex(BREP_VERTEX *vertex1, BREP_VERTEX *vertex2,
                                void (*func)(BREP_WING *)) {
    DoubleLinkedListOfWings *ringel, *next;

    if ( vertex2->wing_ring ) {
        next = vertex2->wing_ring;
        do {
            ringel = next;
            next = ringel->next;
            if ( ringel->wing->contour && ringel->wing->next->vertex == vertex1 ) {
                func(ringel->wing);
            }
        } while ( next && next != vertex2->wing_ring );
    }

    if ( vertex1->wing_ring ) {
        next = vertex1->wing_ring;
        do {
            ringel = next;
            next = ringel->next;
            if ( ringel->wing->contour && ringel->wing->next->vertex == vertex2 ) {
                func(ringel->wing);
            }
        } while ( next && next != vertex1->wing_ring );
    }
}

/* release all storage associated with the vertex. The vertex is supposed not
 * to be used anymore in any edge. Disconnecting vertices and edges happens
 * automatically when destroying edges. */
void BrepDestroyVertex(BREP_VERTEX *vertex) {
    /* check whether the vertex is still being used in edges, if so, refuse to
     * destroy the vertex */
    if ( vertex->wing_ring ) {
        BrepInfo(vertex->client_data, "BrepDestroyVertex", "vertex still being used in edges, will not be destroyed");
        return;
    }

    free(vertex);
}
