/**
Phong type EDFs, BRDFs, BTDFs
*/

#ifndef __PHONG__
#define __PHONG__

#include "common/color.h"
#include "material/xxdf.h"

class PhongBiDirectionalReflectanceDistributionFunction {
  public:
    COLOR Kd;
    COLOR Ks;
    float avgKd;
    float avgKs;
    float Ns;

    PhongBiDirectionalReflectanceDistributionFunction():
        Kd(),
        Ks(),
        avgKd(),
        avgKs(),
        Ns()
    {
    }
};

class PHONG_BTDF {
  public:
    COLOR Kd;
    COLOR Ks;
    float avgKd;
    float avgKs;
    float Ns;
    RefractionIndex refractionIndex;

    PHONG_BTDF():
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

extern PhongBiDirectionalReflectanceDistributionFunction *phongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns);
extern PHONG_BTDF *phongBtdfCreate(COLOR *Kd, COLOR *Ks, float Ns, float nr, float ni);
extern COLOR phongReflectance(PhongBiDirectionalReflectanceDistributionFunction *brdf, char flags);

extern COLOR
phongBrdfEval(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags);

extern Vector3D
phongBrdfSample(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction);

extern void
phongBrdfEvalPdf(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

extern COLOR phongTransmittance(PHONG_BTDF *btdf, char flags);
extern void phongIndexOfRefraction(PHONG_BTDF *btdf, RefractionIndex *index);

extern COLOR
phongBtdfEval(
    PHONG_BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags);

extern Vector3D
phongBtdfSample(
    PHONG_BTDF *btdf,
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
    PHONG_BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR);

#endif
