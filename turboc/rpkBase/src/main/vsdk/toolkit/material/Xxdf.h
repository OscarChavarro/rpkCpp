#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/material/XxdfComponentFlag.h"
#include "vsdk/toolkit/material/BsdfComponent.h"
#include "vsdk/toolkit/material/BsdfComponentFlag.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

class Xxdf {
  public:
    #define PHONG_LOWEST_SPECULAR_EXP ((double)250.0)

    static Vector3Dd
    idealReflectedDirection(const Vector3Dd *in, const Vector3Dd *normal);

    static Vector3Dd
    idealRefractedDirection(
        const Vector3Dd *in,
        const Vector3Dd *normal,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        bool *totalInternalReflection);
};

#endif
