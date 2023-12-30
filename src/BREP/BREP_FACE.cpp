#include <cstdlib>

#include "BREP/BREP_SOLID.h"

/**
Disconnect the face from its containing shell
*/
static void BrepDisconnectFaceFromShell(BREP_FACE *face) {
    BREP_SOLID *shell = face->shell;

    if ( !shell ) {
        // not connected to a shell
        return;
    }

    if ( shell->faces == face ) {
        // the face is the first face in the shell
        if ( face->next == face ) {
            // it is the only face
            shell->faces = (BREP_FACE *) nullptr;
        } else {
            // make the next face the first face of the shell
            shell->faces = face->next;
        }
    }

    face->next->prev = face->prev;
    face->prev->next = face->next;

    face->shell = (BREP_SOLID *) nullptr;
}

/**
Destroys the contours in a face
*/
static void BrepFaceDestroyContours(BREP_CONTOUR *first) {
    BREP_CONTOUR *contour, *prev;

    if ( first ) {
        for ( contour = first->prev; contour != first; contour = prev ) {
            prev = contour->prev;
            BrepDestroyContour(contour);
        }
        BrepDestroyContour(first);
    }
}

/**
Release all storage associated with a face and its contours, including
edges and vertices if not used in other faces as well
*/
void BrepDestroyFace(BREP_FACE *face) {
    // inverse actions performed in BrepCreateFace() in reverse order

    // disconnect the face from the containing shell
    BrepDisconnectFaceFromShell(face);

    // destroy its contours
    BrepFaceDestroyContours(face->outer_contour);

    // dispose of the BREP_FACE structure itself
    free(face);
}
