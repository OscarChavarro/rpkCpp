#ifndef __PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__
#define __PHONG_BIDIRECTIONAL_TRANSMITTANCE_DISTRIBUTION_FUNCTION__

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "material/RefractionIndex.h"

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/
class PhongBidirectionalTransmittanceDistributionFunction {
  public:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;
    RefractionIndex refractionIndex;

    PhongBidirectionalTransmittanceDistributionFunction(const ColorRgb *inKd, const ColorRgb *inKs, float inNs, float inNr, float inNi);

};

extern ColorRgb phongTransmittance(const PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags);
extern void phongIndexOfRefraction(const PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index);

extern ColorRgb
phongBtdfEval(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags);

extern Vector3D
phongBtdfSample(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
phongBtdfEvalPdf(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
