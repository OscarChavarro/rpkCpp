#include "skin/hitlist.h"

/**
Creates a duplicate of the given hit record
*/
RayHit *
duplicateHit(RayHit *hit) {
    RayHit *duplicatedHit = (RayHit *)malloc(sizeof(RayHit));
    *duplicatedHit = *hit;
    return duplicatedHit;
}
