#include "skin/hitlist.h"

/**
Creates a duplicate of the given hit record
*/
RayHit *DuplicateHit(RayHit *hit) {
    RayHit *duplhit = (RayHit *)malloc(sizeof(RayHit));
    *duplhit = *hit;
    return duplhit;
}
