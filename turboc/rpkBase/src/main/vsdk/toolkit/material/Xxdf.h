#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/material/XxdfComponentFlag.h"
#include "vsdk/toolkit/material/BsdfComponent.h"
#include "vsdk/toolkit/material/BsdfComponentFlag.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

class Xxdf {
  public:
    #define PHONG_LOWEST_SPECULAR_EXP ((float)250.0)

    static Vector3Df
    idealReflectedDirection(const Vector3Df *in, const Vector3Df *normal);

    static Vector3Df
    idealRefractedDirection(
        const Vector3Df *in,
        const Vector3Df *normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        bool *totalInternalReflection);
};

#endif
