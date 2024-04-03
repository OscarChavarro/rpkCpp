/**
Bidirectional Transmittance Distribution Functions
*/

#ifndef __BTDF__
#define __BTDF__

#include "material/phong.h"

extern PhongBidirectionalTransmittanceDistributionFunction *
btdfCreate(PhongBidirectionalTransmittanceDistributionFunction *data);

extern COLOR
btdfTransmittance(PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags);

extern void
btdfIndexOfRefraction(PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index);

/************* BTDF Evaluation functions ****************/

/**
All components of the Btdf

Vector directions :

in : towards patch
out : from patch
normal : leaving from patch, on the incoming side.
         So in.normal < 0 !!!
*/

extern COLOR
btdfEval(
    PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags);

extern Vector3D
btdfSample(
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
btdfEvalPdf(
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
