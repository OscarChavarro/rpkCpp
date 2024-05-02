/**
Bidirectional Transmittance Distribution Functions
*/

#ifndef __BTDF__
#define __BTDF__

#include "material/phong.h"

extern ColorRgb
btdfTransmittance(const PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags);

extern void
btdfIndexOfRefraction(const PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index);

/************* BTDF Evaluation functions ****************/

/**
All components of the Btdf

Vector directions :

in : towards patch
out : from patch
normal : leaving from patch, on the incoming side.
         So in.normal < 0 !!!
*/

extern ColorRgb
btdfEval(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags);

extern Vector3D
btdfSample(
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
btdfEvalPdf(
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
