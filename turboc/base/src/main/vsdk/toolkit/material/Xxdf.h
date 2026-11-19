#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/XxdfComponentFlag.h"
#include "vsdk/toolkit/material/BsdfComponent.h"
#include "vsdk/toolkit/material/BsdfComponentFlag.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

class Xxdf {
  public:
    #define PHONG_LOWEST_SPECULAR_EXP ((float)250.0)

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
