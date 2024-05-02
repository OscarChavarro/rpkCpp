/**
Phong type EDFs, BRDFs, BTDFs
*/

#ifndef __PHONG__
#define __PHONG__

#include "common/ColorRgb.h"
#include "material/xxdf.h"

class PhongBidirectionalReflectanceDistributionFunction {
  public:
    ColorRgb Kd;
    ColorRgb Ks;
    float avgKd;
    float avgKs;
    float Ns;

    PhongBidirectionalReflectanceDistributionFunction():
        Kd(),
        Ks(),
        avgKd(),
        avgKs(),
        Ns()
    {
    }
};

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

    PhongBidirectionalTransmittanceDistributionFunction():
        Kd(),
        Ks(),
        avgKd(),
        avgKs(),
        Ns(),
        refractionIndex()
    {
    }
};

/**
Define the phong exponent making the difference
between glossy and highly specular reflection/transmission.
Choice is arbitrary for the moment
*/

#define PHONG_LOWEST_SPECULAR_EXP 250
#define PHONG_IS_SPECULAR(p) ((p).Ns >= PHONG_LOWEST_SPECULAR_EXP)

extern PhongBidirectionalReflectanceDistributionFunction *phongBrdfCreate(const ColorRgb *Kd, const ColorRgb *Ks, double Ns);
extern PhongBidirectionalTransmittanceDistributionFunction *phongBtdfCreate(const ColorRgb *Kd, const ColorRgb *Ks, float Ns, float nr, float ni);
extern ColorRgb phongReflectance(const PhongBidirectionalReflectanceDistributionFunction *brdf, char flags);

extern ColorRgb
phongBrdfEval(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags);

extern Vector3D
phongBrdfSample(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
phongBrdfEvalPdf(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

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
