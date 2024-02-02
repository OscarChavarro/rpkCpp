#ifndef __BACKGROUND__
#define __BACKGROUND__

#include "common/linealAlgebra/Vector3D.h"
#include "common/color.h"
#include "skin/Patch.h"

class BACKGROUND_METHODS {
public:
    /* evaluate background radiance coming in from direction (direction
     * positions towards the background). If 'pdf' is non-null, also fills
     * in the probability of sampling this direction with Sample() */
    COLOR (*Radiance)(void *data, Vector3D *position, Vector3D *direction, float *pdf);

    /* Samples a direction to the background, taking into account the
     * radiance coming in from the background. The returned direction
     * is unique for given xi1, xi2 (in the range [0,1), including 0 but
     * excluding 1). Directions on a full sphere may be returned. If a
     * direction is inappropriate, a new direction (with new numbers xi1, xi2)
     * needs to be sampled. If value or pdf is non-null, the radiance coming
     * in from the sampled direction or the probability of sampling the
     * direction are computed on the fly. */
    Vector3D (*Sample)(void *data, Vector3D *position, float xi1, float xi2, COLOR *radiance, float *pdf);

    /* Computes total power emitted by the background (= integral over
     * the full sphere of the background radiance */
    COLOR (*Power)(void *data, Vector3D *position);

    void (*Print)(FILE *out, void *data);

    void (*Destroy)(void *data);
};

class Background {
  public:
    void *data; // Object state
    Patch *bkgPatch; // Virtual patch for background
    BACKGROUND_METHODS *methods; // class methods operating on state
};

extern COLOR backgroundRadiance(Background *bkg, Vector3D *position, Vector3D *direction, float *pdf);
extern COLOR backgroundPower(Background *bkg, Vector3D *position);

#endif
