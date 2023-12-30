#include "skin/hitlist.h"

/**
Creates a duplicate of the given hit record
*/
HITREC *DuplicateHit(HITREC *hit) {
    HITREC *duplhit = (HITREC *)malloc(sizeof(HITREC));
    *duplhit = *hit;
    return duplhit;
}
