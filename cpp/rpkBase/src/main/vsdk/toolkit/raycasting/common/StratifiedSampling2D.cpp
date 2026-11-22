#include <cstdlib>

#include "java/lang/Math.h"
#include "vsdk/toolkit/raycasting/common/StratifiedSampling2D.h"

StratifiedSampling2D::StratifiedSampling2D(int nrSamples): xMaxStratum(), yMaxStratum() {
    getNumberOfDivisions(nrSamples, &xMaxStratum, &yMaxStratum);
    xStratum = 0;
    yStratum = 0;
}

namespace {

// SplitMix64 finalizer. In parallel mode each pixel has a separate range of
// sample indices, so scheduling does not affect the samples it receives.
double
sampleUnitInterval(uint64_t *sampleIndex) {
    if ( sampleIndex == nullptr ) {
        return drand48();
    }
    uint64_t bits = (*sampleIndex)++;
    bits = (bits ^ (bits >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    bits = (bits ^ (bits >> 27)) * UINT64_C(0x94d049bb133111eb);
    bits ^= bits >> 31;
    return static_cast<double>(bits >> 11) / 9007199254740992.0;
}

}

void
StratifiedSampling2D::sample(double *x1, double *x2, uint64_t *sampleIndex) {
    if ( yStratum < yMaxStratum ) {
        *x1 = ((xStratum + sampleUnitInterval(sampleIndex)) / static_cast<double>(xMaxStratum));
        *x2 = ((yStratum + sampleUnitInterval(sampleIndex)) / static_cast<double>(yMaxStratum));

        if ( (++xStratum) == xMaxStratum ) {
            xStratum = 0;
            yStratum++;
        }
    } else {
        // All strata sampled -> now just uniform sampling
        *x1 = sampleUnitInterval(sampleIndex);
        *x2 = sampleUnitInterval(sampleIndex);
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
