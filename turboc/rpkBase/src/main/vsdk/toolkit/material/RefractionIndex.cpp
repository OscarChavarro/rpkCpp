#include "java/lang/Math.h"
#include "vsdk/toolkit/material/RefractionIndex.h"

/**
Compute an approximate geometric IOR from a complex IOR (cfr. Gr.Gems II, p289)
*/
double
RefractionIndex::cmplxTGmtrcRfrctIndx() const {
    double f1 = (nr - 1.0);
    f1 = f1 * f1 + ni * ni;

    double f2 = (nr + 1.0);
    f2 = f2 * f2 + ni * ni;

    double sqrtF = Math::sqrt(f1 / f2);

    return (1.0 + sqrtF) / (1.0 - sqrtF);
}
