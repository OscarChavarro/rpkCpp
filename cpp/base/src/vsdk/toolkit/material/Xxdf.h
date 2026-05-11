/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef XXDF__
#define XXDF__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/XxdfComponentFlag.h"
#include "vsdk/toolkit/material/BsdfComponent.h"
#include "vsdk/toolkit/material/BsdfComponentFlag.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

class Xxdf {
  public:
    static constexpr float PHONG_LOWEST_SPECULAR_EXP = 250.0F;

    static Vector3D
    idealReflectedDirection(const Vector3D *in, const Vector3D *normal);

    static Vector3D
    idealRefractedDirection(
        const Vector3D *in,
        const Vector3D *normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        bool *totalInternalReflection);
};

#endif
