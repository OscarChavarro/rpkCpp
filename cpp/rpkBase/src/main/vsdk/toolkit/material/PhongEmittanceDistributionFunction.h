#ifndef __PHONG_EMITTANCE_DISTRIBUTION_FUNCTION__
#define __PHONG_EMITTANCE_DISTRIBUTION_FUNCTION__

#include "vsdk/toolkit/environment/material/PhongMaterial.h"
#include "vsdk/toolkit/material/ShadingContext.h"
#include "vsdk/toolkit/material/Xxdf.h"

/**
RenderPark's view of the self-emitted radiance distribution of a light
source material, with RenderPark's argument types: every function converts
its arguments and answers through the VITRAL PhongMaterial, whose emission
lobe is the model (plan step 2b).

Read-only after construction; does not own the PhongMaterial.
*/
class PhongEmittanceDistributionFunction {
  private:
    const PhongMaterial *material;

  public:
    explicit PhongEmittanceDistributionFunction(const PhongMaterial *inMaterial);
    virtual ~PhongEmittanceDistributionFunction();

    static bool edfIsTextured();
    static bool
    edfShadingFrame(
        const ShadingContext &context,
        const Vector3Dd *X,
        const Vector3Dd *Y,
        const Vector3Dd *Z);

    /** Emittance (self-emitted radiant exitance) [W / m^2] of the components in flags */
    ColorRgb phongEmittance(const ShadingContext *context, char flags) const;

    /**
    Exitant radiance [W / m^2 sr] into the direction out; when
    probabilityDensityFunction is not null, the density of out under
    phongEdfSample goes there
    */
    ColorRgb
    phongEdfEval(
        const ShadingContext *context,
        const Vector3Dd *out,
        char flags,
        double *probabilityDensityFunction) const;

    /**
    Samples an emitted direction; the radiance along it and its density go to
    the non-null pointers
    */
    Vector3Dd
    phongEdfSample(
        const ShadingContext *context,
        char flags,
        double xi1,
        double xi2,
        ColorRgb *selfEmittedRadiance,
        double *probabilityDensityFunction) const;
};

#endif
