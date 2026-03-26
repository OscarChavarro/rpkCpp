/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "common/linealAlgebra/Vector3D.h"
#include "material/RefractionIndex.h"
#include "material/BsdfComponentFlags.h"

extern const float PHONG_LOWEST_SPECULAR_EXP;

extern Vector3D idealReflectedDirection(const Vector3D *in, const Vector3D *normal);

extern Vector3D
idealRefractedDirection(
    const Vector3D *in,
    const Vector3D *normal,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    bool *totalInternalReflection);

#endif
