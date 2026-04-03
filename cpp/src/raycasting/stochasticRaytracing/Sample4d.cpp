/**
4D vector sampling
*/

#include <cstdlib>

#include "common/Error.h"
#include "numericalAnalysis/quasiMonteCarlo/Faure.h"
#include "numericalAnalysis/quasiMonteCarlo/Halton.h"
#include "numericalAnalysis/quasiMonteCarlo/Niederreiter31.h"
#include "numericalAnalysis/quasiMonteCarlo/ScrambledHalton.h"
#include "numericalAnalysis/quasiMonteCarlo/Sobol.h"
#include "raycasting/stochasticRaytracing/Sample4d.h"

Sampler4DSequence Sample4d::seq = Sampler4DSequence::RANDOM;

const char *
Sample4d::sequenceName(Sampler4DSequence sequence) {
    switch ( sequence ) {
        case Sampler4DSequence::RANDOM:
            return RANDOM_NAME;
        case Sampler4DSequence::HALTON:
            return HALTON_NAME;
        case Sampler4DSequence::SCRAMBLED_HALTON:
            return SCRAMBLED_HALTON_NAME;
        case Sampler4DSequence::SOBOL:
            return SOBOL_NAME;
        case Sampler4DSequence::ORIGINAL_FAURE:
            return ORIGINAL_FAURE_NAME;
        case Sampler4DSequence::GENERALIZED_FAURE:
            return GENERALIZED_FAURE_NAME;
        case Sampler4DSequence::NIEDERREITER:
            return NIEDERREITER_NAME;
        default:
            return UNKNOWN_NAME;
    }
}

/**
Also initialises the sequence
*/
void
Sample4d::setSequence4D(Sampler4DSequence sequence) {
    seq = sequence;
    switch ( seq ) {
        case Sampler4DSequence::SOBOL:
            Sobol::initSobol(4);
            break;
        case Sampler4DSequence::ORIGINAL_FAURE:
            Faure::initOriginalFaureSequence(4);
            break;
        case Sampler4DSequence::GENERALIZED_FAURE:
            Faure::initGeneralizedFaureSequence(4);
            break;
        default:
            break;
    }
}

/**
Returns 4D sample with given index from current sequence. When the
current sequence is 'random', the index is not used
*/
double *
Sample4d::sample4D(unsigned seed) {
    static double xi[4];
    const unsigned *zeta;
    const double *xx;

    switch ( seq ) {
        case Sampler4DSequence::RANDOM:
            xi[0] = drand48();
            xi[1] = drand48();
            xi[2] = drand48();
            xi[3] = drand48();
            break;
        case Sampler4DSequence::HALTON:
            xi[0] = Halton::Halton2(static_cast<int>(seed));
            xi[1] = Halton::Halton3(static_cast<int>(seed));
            xi[2] = Halton::Halton5(static_cast<int>(seed));
            xi[3] = Halton::Halton7(static_cast<int>(seed));
            break;
        case Sampler4DSequence::SCRAMBLED_HALTON:
            xx = ScrambledHalton::scrambledHalton(seed, 4);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::SOBOL:
            xx = Sobol::sobol(static_cast<int>(seed));
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::ORIGINAL_FAURE:
        case Sampler4DSequence::GENERALIZED_FAURE:
            xx = Faure::faure(static_cast<int>(seed));
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::NIEDERREITER:
            zeta = Niederreiter31::niederreiter31(seed);
            xi[0] = zeta[0] * Niederreiter31::RECIP;
            xi[1] = zeta[1] * Niederreiter31::RECIP;
            xi[2] = zeta[2] * Niederreiter31::RECIP;
            xi[3] = zeta[3] * Niederreiter31::RECIP;
            break;
        default:
            Error::fatal(-1, "Sample4d::sample4D", "QMC Sequence %s not yet implemented", sequenceName(seq));
    }

    return xi;
}

/**
The following routines are safe with Sample4D(), which calls only
31-bit sequences (including 31-bit Niederreiter sequence). If
you are looking for such a routine to use directly in conjunction
with the routine Niederreiter::Nied() or Niederreiter::NextNiedInRange(), you should use
the Niederreiter::foldSample() routine in Niederreiter.h instead.
Niederreiter::Nied() and Niederreiter::NextNiedInRange() are 63-bit unless compiled without
'unsigned long long' support
*/
void
Sample4d::foldSampleU(unsigned *xi1, unsigned *xi2) {
    Niederreiter31::foldSample31(xi1, xi2);
}

void
Sample4d::foldSampleF(double *xi1, double *xi2) {
    unsigned zeta1 = static_cast<unsigned>(*xi1 * Niederreiter31::RECIP1);
    unsigned zeta2 = static_cast<unsigned>(*xi2 * Niederreiter31::RECIP1);
    Sample4d::foldSampleU(&zeta1, &zeta2);
    *xi1 = zeta1 * Niederreiter31::RECIP;
    *xi2 = zeta2 * Niederreiter31::RECIP;
}
