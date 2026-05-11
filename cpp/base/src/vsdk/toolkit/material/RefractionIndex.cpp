#include "vsdk/toolkit/java/lang/Math.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

/**
Compute an approximate geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/
float
RefractionIndex::complexToGeometricRefractionIndex() const {
    float f1 = (nr - 1.0F);
    f1 = f1 * f1 + ni * ni;

    float f2 = (nr + 1.0F);
    f2 = f2 * f2 + ni * ni;

    float sqrtF = java::Math::sqrt(f1 / f2);

    return (1.0F + sqrtF) / (1.0F - sqrtF);
}
