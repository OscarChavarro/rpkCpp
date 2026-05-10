#ifndef __BACKGROUND__
#define __BACKGROUND__

#include "common/linealAlgebra/Vector3D.h"
#include "common/color/ColorRgb.h"
#include "common/RenderOptions.h"
#include "environment/geometry/elements/Patch.h"

class Background {
  public:
    Background();
    virtual ~Background();

    /*
    Evaluate background radiance coming in from direction (direction
    positions towards the background). If probabilityDensityFunction is non-null, also fills
    in the probability of sampling this direction with sample()
    */
    virtual ColorRgb
    radiance(Vector3D *position, Vector3D *direction, float *probabilityDensityFunction) const;

    /*
    Samples a direction to the background, taking into account the
    radiance coming in from the background. The returned direction
    is unique for given xi1, xi2 (in the range [0,1), including 0 but
    excluding 1). Directions on a full sphere may be returned. If a
    direction is inappropriate, a new direction (with new numbers xi1, xi2)
    needs to be sampled. If value or pdf is non-null, the radiance coming
    in from the sampled direction or the probability of sampling the
    direction are computed on the fly.
    */
    virtual Vector3D
    sample(
        Vector3D *position,
        float xi1,
        float xi2,
        ColorRgb *radiance,
        float *probabilityDensityFunction) const;

    /*
    Computes total power emitted by the background (= integral over
    the full sphere of the background radiance.
    */
    virtual ColorRgb
    power(Vector3D *position) const;

#ifdef RAYTRACING_ENABLED
    static ColorRgb
    backgroundRadiance(
        Background *bkg,
        Vector3D *position,
        Vector3D *direction,
        float *probabilityDensityFunction);
#endif

    static ColorRgb
    backgroundPower(Background *bkg, Vector3D *position);

    Patch *bkgPatch; // Virtual patch for background
};

#endif
