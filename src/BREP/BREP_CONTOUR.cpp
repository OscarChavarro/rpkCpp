#include <cstdlib>

#include "BREP/BREP_SOLID.h"

/**
Disconnects a contour from its containing face
*/
void
brepDisconnectContourFromFace(BREP_CONTOUR *contour) {
    BREP_FACE *face = contour->face;

    if ( face->outer_contour == contour ) {
        // The contour is the first contour in the face
        if ( contour->next == contour ) {
            // It is the only contour
            face->outer_contour = (BREP_CONTOUR *) nullptr;
        } else {
            // The faces looses its outer contour
            face->outer_contour = contour->next;
        }
    }

    contour->next->prev = contour->prev;
    contour->prev->next = contour->next;

    contour->face = (BREP_FACE *) nullptr;
}

/**
Destroys the wings in a contour
*/
static void
brepContourDestroyWings(BREP_WING *first) {
    BREP_WING *wing, *prev;

    if ( first ) {
        for ( wing = first->prev; wing != first; wing = prev ) {
            prev = wing->prev;
            brepDestroyWing(wing);
        }
        brepDestroyWing(first);
    }
}

/**
Release all storage associated with a contour and its contours,
including edges and vertices if not used in other contours as well
*/
void
brepDestroyContour(BREP_CONTOUR *contour) {
    // Disconnect the contour from the containing face
    brepDisconnectContourFromFace(contour);

    // Destroy its wings
    brepContourDestroyWings(contour->wings);

    // Dispose of the BREP_CONTOUR structure itself
    free(contour);
}
