/**
Phong type EDFs, BRDFs, BTDFs
*/

#ifndef __PHONG__
#define __PHONG__

#include "common/color.h"
#include "material/xxdf.h"

class PhongBidirectionalReflectanceDistributionFunction {
  public:
    COLOR Kd;
    COLOR Ks;
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
    COLOR Kd;
    COLOR Ks;
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

extern PhongBidirectionalReflectanceDistributionFunction *phongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns);
extern PhongBidirectionalTransmittanceDistributionFunction *phongBtdfCreate(COLOR *Kd, COLOR *Ks, float Ns, float nr, float ni);
extern COLOR phongReflectance(PhongBidirectionalReflectanceDistributionFunction *brdf, char flags);

extern COLOR
phongBrdfEval(
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        char flags);

extern Vector3D
phongBrdfSample(
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

extern void
phongBrdfEvalPdf(
        PhongBidirectionalReflectanceDistributionFunction *brdf,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);

extern COLOR phongTransmittance(PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags);
extern void phongIndexOfRefraction(PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index);

extern COLOR
phongBtdfEval(
        PhongBidirectionalTransmittanceDistributionFunction *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        char flags);

extern Vector3D
phongBtdfSample(
        PhongBidirectionalTransmittanceDistributionFunction *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        char flags,
        double x1,
        double x2,
        double *probabilityDensityFunction);

extern void
phongBtdfEvalPdf(
        PhongBidirectionalTransmittanceDistributionFunction *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        char flags,
        double *probabilityDensityFunction,
        double *probabilityDensityFunctionRR);

#endif
