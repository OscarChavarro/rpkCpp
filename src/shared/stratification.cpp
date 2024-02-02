#include <cmath>

#include "scene/spherical.h"
#include "shared/stratification.h"

CStrat2D::CStrat2D(int nrSamples) {
    getNumberOfDivisions(nrSamples, &xMaxStratum, &yMaxStratum);
    xStratum = 0;
    yStratum = 0;
}

void
CStrat2D::sample(double *x1, double *x2) {
    if ( yStratum < yMaxStratum ) {
        *x1 = ((xStratum + drand48()) / (double) xMaxStratum);
        *x2 = ((yStratum + drand48()) / (double) yMaxStratum);

        if ( (++xStratum) == xMaxStratum ) {
            xStratum = 0;
            yStratum++;
        }
    } else {
        // All strata sampled -> now just uniform sampling
        *x1 = drand48();
        *x2 = drand48();
    }
}
