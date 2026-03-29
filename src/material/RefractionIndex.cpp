#include "java/lang/Math.h"
#include "material/RefractionIndex.h"

/**
Compute an approximate geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/
float
RefractionIndex::complexToGeometricRefractionIndex() const {
    float f1 = (nr - 1.0f);
    f1 = f1 * f1 + ni * ni;

    float f2 = (nr + 1.0f);
    f2 = f2 * f2 + ni * ni;

    float sqrtF = java::Math::sqrt(f1 / f2);

    return (1.0f + sqrtF) / (1.0f - sqrtF);
}
