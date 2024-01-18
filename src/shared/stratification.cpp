#include <cmath>

#include "scene/spherical.h"
#include "shared/stratification.h"

CStrat2D::CStrat2D(int nrSamples) {
    GetNrDivisions(nrSamples, &xMaxStratum, &yMaxStratum);
    totalSamples = nrSamples;
    currentSample = 0;
    xStratum = 0;
    yStratum = 0;
}

void CStrat2D::Init() {
    currentSample = 0;
}

void
CStrat2D::Sample(double *x_1, double *x_2) {
    if ( yStratum < yMaxStratum ) {
        *x_1 = ((xStratum + drand48()) / (double) xMaxStratum);
        *x_2 = ((yStratum + drand48()) / (double) yMaxStratum);

        if ((++xStratum) == xMaxStratum ) {
            xStratum = 0;
            yStratum++;
        }
    } else {
        // All strata sampled -> now just uniform sampling
        *x_1 = drand48();
        *x_2 = drand48();
    }
}
