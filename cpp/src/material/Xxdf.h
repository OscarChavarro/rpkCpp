/**
General definitions for edf, brdf, btdf, etc.
*/

#ifndef XXDF__
#define XXDF__

#include "common/linealAlgebra/Vector3D.h"
#include "material/XxdfComponentFlag.h"
#include "material/BsdfComponent.h"
#include "material/BsdfComponentFlag.h"
#include "material/RefractionIndex.h"

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
