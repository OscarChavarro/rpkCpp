#include <cstdlib>

#include "BREP/BREP_SOLID.h"

/* disconnects a contour from its containing face */
void BrepDisconnectContourFromFace(BREP_CONTOUR *contour) {
    BREP_FACE *face = contour->face;

    if ( face->outer_contour == contour ) {
        // the contour is the first contour in the face
        if ( contour->next == contour ) {
            // it is the only contour
            face->outer_contour = (BREP_CONTOUR *) nullptr;
        } else {
            // the faces looses its outer contour
            face->outer_contour = contour->next;
        }
    }

    contour->next->prev = contour->prev;
    contour->prev->next = contour->next;

    contour->face = (BREP_FACE *) nullptr;
}

/* destroys the wings in a contour */
static void BrepContourDestroyWings(BREP_WING *first) {
    BREP_WING *wing, *prev;

    if ( first ) {
        for ( wing = first->prev; wing != first; wing = prev ) {
            prev = wing->prev;
            BrepDestroyWing(wing);
        }
        BrepDestroyWing(first);
    }
}

/* release all storage associated with a contour and its contours, 
 * including edges and vertices if not used in other contours as well */
void BrepDestroyContour(BREP_CONTOUR *contour) {
    /* inverse actions performed in BrepCreateContour() in reverse order */

    /* disconnect the contour from the containing face */
    BrepDisconnectContourFromFace(contour);

    /* destroy its wings */
    BrepContourDestroyWings(contour->wings);

    /* dispose of the BREP_CONTOUR structure itself */
    free(contour);
}
