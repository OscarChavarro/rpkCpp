#include <cstdlib>

#include "material/PhongEmittanceDistributionFunctions.h"
#include "common/error.h"

/**
Creates a EDF instance with given data and methods. A pointer
to the created EDF object is returned
*/
PhongEmittanceDistributionFunctions *
edfCreate(PHONG_EDF *data, EDF_METHODS *methods) {
    PhongEmittanceDistributionFunctions *edf = (PhongEmittanceDistributionFunctions *)malloc(sizeof(PhongEmittanceDistributionFunctions));
    edf->data = data;
    edf->methods = methods;

    return edf;
}

/**
Returns the emittance (self-emitted radiant exitance) [W/m^2] of the EDF
*/
COLOR
edfEmittance(PhongEmittanceDistributionFunctions *edf, RayHit *hit, XXDFFLAGS flags) {
    if ( edf && edf->methods->Emittance ) {
        return edf->methods->Emittance(edf->data, hit, flags);
    } else {
        static COLOR emit;
        colorClear(emit);
        return emit;
    }
}

int
edfIsTextured(PhongEmittanceDistributionFunctions *edf) {
    if ( edf && edf->methods->IsTextured ) {
        return edf->methods->IsTextured(edf->data);
    }
    return false;
}

/**
Evaluates the edf: return exitant radiance [W/m^2 sr] into the direction
out. If probabilityDensityFunction is not null, the stochasticJacobiProbability density of the direction is
computed and returned in probabilityDensityFunction
*/
COLOR
edfEval(PhongEmittanceDistributionFunctions *edf, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *probabilityDensityFunction) {
    if ( edf && edf->methods->evaluate ) {
        return edf->methods->evaluate(edf->data, hit, out, flags, probabilityDensityFunction);
    } else {
        static COLOR val;
        colorClear(val);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        return val;
    }
}

/**
Samples a direction according to the specified edf and emission range determined
by flags. If emitted_radiance is not a null pointer, the emitted radiance along
the generated direction is returned. If probabilityDensityFunction is not null, the stochasticJacobiProbability density
of the generated direction is computed and returned in probabilityDensityFunction
*/
Vector3D
edfSample(PhongEmittanceDistributionFunctions *edf, RayHit *hit, XXDFFLAGS flags,
          double xi1, double xi2,
          COLOR *emitted_radiance, double *probabilityDensityFunction) {
    if ( edf && edf->methods->Sample ) {
        return edf->methods->Sample(edf->data, hit, flags, xi1, xi2, emitted_radiance, probabilityDensityFunction);
    } else {
        Vector3D v = {0.0, 0.0, 0.0};
        logFatal(-1, "edfSample", "Can't sample EDF");
        return v;
    }
}

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: also edf's can have a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - hitPointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
int
edfShadingFrame(PhongEmittanceDistributionFunctions *edf, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    if ( edf && edf->methods->ShadingFrame ) {
        return edf->methods->ShadingFrame(edf->data, hit, X, Y, Z);
    }
    return false;
}
