#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfFaceWithHolesEntityExpander.h"

/**
Replace face + holes with single contour
*/
int
MgfFaceWithHolesEntityExpander::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    const char *newArgumentValues[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastPerimeterIndex = 0;

    newArgumentValues[0] = context->entityNames[FACE];
    int i;
    for ( i = 1; i < argumentCount; i++ ) {
        if ( argumentValues[i][0] == '-' ) {
            if ( i < 4 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            if ( i >= argumentCount - 1 ) {
                break;
            }
            if ( !lastPerimeterIndex ) {
                lastPerimeterIndex = i - 1;
            }
            int j;
            for ( j = i + 1; j < argumentCount - 1 && argumentValues[j + 1][0] != '-'; j++ ) {}
            if ( j - i < 3 ) {
                return MGF_ERRR_WRNG_NUM_O_ARGMN;
            }
            newArgumentValues[i] = argumentValues[j]; // Connect hole loop
        } else {
            // Hole or perimeter vertex
            newArgumentValues[i] = argumentValues[i];
        }
    }
    if ( lastPerimeterIndex ) {
        // Finish seam to outside
        newArgumentValues[i++] = argumentValues[lastPerimeterIndex];
    }
    newArgumentValues[i] = NULL;
    return MgfEntityControl::mgfHandle(FACE, i, newArgumentValues, context);
}
