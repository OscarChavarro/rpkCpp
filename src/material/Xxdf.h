/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef __XXDF__
#define __XXDF__

#include "common/linealAlgebra/Vector3D.h"
#include "material/XxdfComponentFlag.h"
#include "material/BsdfComponent.h"
#include "material/BsdfComponentFlag.h"
#include "material/RefractionIndex.h"

extern const float PHONG_LOWEST_SPECULAR_EXP;

class Xxdf {
  public:
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
