#include <cstdlib>

#include "vsdk/toolkit/java/lang/Math.h"
#include "StratifiedSampling2D.h"

StratifiedSampling2D::StratifiedSampling2D(int nrSamples): xMaxStratum(), yMaxStratum() {
    getNumberOfDivisions(nrSamples, &xMaxStratum, &yMaxStratum);
    xStratum = 0;
    yStratum = 0;
}

void
StratifiedSampling2D::sample(double *x1, double *x2) {
    if ( yStratum < yMaxStratum ) {
        *x1 = ((xStratum + drand48()) / static_cast<double>(xMaxStratum));
        *x2 = ((yStratum + drand48()) / static_cast<double>(yMaxStratum));

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

/**
Makes a nice grid for stratified sampling
*/
void
StratifiedSampling2D::getNumberOfDivisions(int samples, int *divs1, int *divs2) {
    if ( samples <= 0 ) {
        *divs1 = 0;
        *divs2 = 0;
        return;
    }

    *divs1 = static_cast<int>(java::Math::ceil(java::Math::sqrt(static_cast<double>(samples))));
    *divs2 = samples / (*divs1);
    while ( (*divs1) * (*divs2) != samples && (*divs1) > 1 ) {
        (*divs1)--;
        *divs2 = samples / (*divs1);
    }
}
