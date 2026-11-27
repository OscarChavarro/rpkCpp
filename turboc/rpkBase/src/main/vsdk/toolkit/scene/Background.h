#ifndef __BACKGROUND__
#define __BACKGROUND__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Background {
  public:
    Background();
    virtual ~Background();

    /*
    Evaluate background radiance coming in from direction (direction
    positions towards the background). If probabilityDensityFunction is non-null, also fills
    in the probability of sampling this direction with sample()
    */
    virtual ColorRgbMutable
    radiance(Vector3Dd *position, Vector3Dd *direction, double *probabilityDensityFunction) const;

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
    virtual Vector3Dd
    sample(
        Vector3Dd *position,
        double xi1,
        double xi2,
        ColorRgbMutable *radiance,
        double *probabilityDensityFunction) const;

    /*
    Computes total power emitted by the background (= integral over
    the full sphere of the background radiance.
    */
    virtual ColorRgbMutable
    power(Vector3Dd *position) const;

#ifdef RAYTRACING_ENABLED
    static ColorRgbMutable
    backgroundRadiance(
        Background *bkg,
        Vector3Dd *position,
        Vector3Dd *direction,
        double *probabilityDensityFunction);
#endif

    static ColorRgbMutable
    backgroundPower(Background *bkg, Vector3Dd *position);

    Patch *bkgPatch; // Virtual patch for background
};

#endif
