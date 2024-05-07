/**
Bidirectional Reflectance Distribution Functions
*/

#include "material/BidirectionalScatteringDistributionFunction.h"
#include "material/SplitBidirectionalScatteringDistributionFunction.h"

/**
Creates a BSDF instance with given data and methods
*/
BidirectionalScatteringDistributionFunction::BidirectionalScatteringDistributionFunction(
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        PhongBidirectionalTransmittanceDistributionFunction *btdf,
        Texture *texture)
{
    this->brdf = brdf;
    this->btdf = btdf;
    this->texture = texture;
}

BidirectionalScatteringDistributionFunction::~BidirectionalScatteringDistributionFunction() {
    if ( brdf != nullptr ) {
        delete brdf;
        brdf = nullptr;
    }

    if ( btdf != nullptr ) {
        delete btdf;
        btdf = nullptr;
    }

    if ( texture != nullptr ) {
        delete texture;
    }
}

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: edf can have also a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - pointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
bool
BidirectionalScatteringDistributionFunction::bsdfShadingFrame(
    const RayHit * /*hit*/,
    const Vector3D * /*X*/,
    const Vector3D * /*Y*/,
    const Vector3D * /*Z*/)
{
    // Not implemented, should call to bsdf->methods->shadingFrame or something like that
    return false;
}

/**
bsdfEvalComponents :
Evaluates all requested components of the BSDF separately and
stores the result in 'colArray'.
Total evaluation is returned.
*/
ColorRgb
BidirectionalScatteringDistributionFunction::bsdfEvalComponents(
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    const char flags,
    ColorRgb *colArray) const
{
    // Some caching optimisation could be used here
    ColorRgb result;
    ColorRgb empty;
    char thisFlag;

    empty.clear();
    result.clear();

    for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
        thisFlag = (char)BSDF_INDEX_TO_COMP(i);

        if ( flags & thisFlag ) {
            colArray[i] = SplitBidirectionalScatteringDistributionFunction::evaluate(
                this, hit, inBsdf, outBsdf, in, out, thisFlag);
            result.add(result, colArray[i]);
        } else {
            colArray[i] = empty;  // Set to 0 for safety
        }
    }

    return result;
}
