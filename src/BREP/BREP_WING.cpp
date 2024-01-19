#include "BREP/BREP_SOLID.h"
#include "BREP/brep_log.h"

/**
Disconnect the wing from the contour. Use with care:
   1. the contour might not be a loop anymore after disconnecting the wing!
   2. the contour might contain holes after disconnecting the wing!
wing->next and wing->prev are left untouched as these fields are needed
when disconnecting an edge from its vertices.
*/
void
BrepDisconnectWingFromContour(BREP_WING *wing) {
    BREP_CONTOUR *contour = wing->contour;

    if ( !contour || !wing->next || !wing->prev ) {
        BrepError(wing->edge->client_data, "BrepDisconnectWingFromContour", "wing improperly connected to contour");
    }

    if ( contour && contour->wings == wing ) {    /* the wing is the first wing
						 * in the contour */
        if ( wing->next == wing ) {            /* it is the only wing */
            contour->wings = (BREP_WING *) nullptr;
        } else {        /* make the next wing the first wing of the contour */
            contour->wings = wing->next;
        }
    }

    /* endpoint of wing->prev will in general not be the same as
     * startpoint of wing->next! A hole might be created in the contour. */
    if ( wing->next ) {
        wing->next->prev = wing->prev;
    }
    if ( wing->prev ) {
        wing->prev->next = wing->next;
    }

    wing->contour = (BREP_CONTOUR *) nullptr;
    wing->next = wing->prev = (BREP_WING *) nullptr;
}

/**
Remove a wing from a contour, release the storage associated with the
edge if it is not used in other contours as well.

This routine will in general create a hole in a contour: the endpoint
of a previous edge might not be the starting point of the next one where
an edge has been deleted. The vertices are not deleted.
remove a wing from a contour, release the storage associated with the
edge and its vertices if it is not used in other contours as well.
Inverse of BrepCreateWing(), but also destroys unused vertices.
*/
void BrepDestroyWing(BREP_WING *wing) {
    /* first disconnect the wing from its starting vertex */
    brepDisconnectWingFromVertex(wing);

    /* then disconnect the wing from its contour */
    BrepDisconnectWingFromContour(wing);

    /* destroy the edge if not being used in any contour anymore
     * (BrepDestroyEdge already checks for this). */
    BrepDestroyEdge(wing->edge);
}
