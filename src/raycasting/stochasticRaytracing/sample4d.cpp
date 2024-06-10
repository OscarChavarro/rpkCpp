/**
4D vector sampling
*/

#include <cstdlib>

#include "common/error.h"
#include "common/quasiMonteCarlo/Halton.h"
#include "common/quasiMonteCarlo/ScrambledHalton.h"
#include "common/quasiMonteCarlo/Sobol.h"
#include "common/quasiMonteCarlo/Faure.h"
#include "common/quasiMonteCarlo/Niederreiter31.h"
#include "raycasting/stochasticRaytracing/sample4d.h"

static Sampler4DSequence seq = Sampler4DSequence::RANDOM;

static const char RANDOM_NAME[8] = "drand48";
static const char HALTON_NAME[7] = "Halton";
static const char SCRAMBLED_HALTON_NAME[12] = "ScramHalton";
static const char SOBOL_NAME[6] = "sobol";
static const char ORIGINAL_FAURE_NAME[6] = "faure";
static const char GENERALIZED_FAURE_NAME[7] = "GFaure";
static const char NIEDERREITER_NAME[5] = "Nied";
static const char UNKNOWN_NAME[8] = "Unknown";

static inline const char *
SEQ4D_NAME(Sampler4DSequence sequence) {
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
setSequence4D(Sampler4DSequence sequence) {
    seq = sequence;
    switch ( seq ) {
        case Sampler4DSequence::SOBOL:
            initSobol(4);
            break;
        case Sampler4DSequence::ORIGINAL_FAURE:
            initOriginalFaureSequence(4);
            break;
        case Sampler4DSequence::GENERALIZED_FAURE:
            initGeneralizedFaureSequence(4);
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
sample4D(unsigned seed) {
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
            xi[0] = Halton2((int)seed);
            xi[1] = Halton3((int)seed);
            xi[2] = Halton5((int)seed);
            xi[3] = Halton7((int)seed);
            break;
        case Sampler4DSequence::SCRAMBLED_HALTON:
            xx = scrambledHalton(seed, 4);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::SOBOL:
            xx = sobol((int)seed);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::ORIGINAL_FAURE:
        case Sampler4DSequence::GENERALIZED_FAURE:
            xx = faure((int) seed);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case Sampler4DSequence::NIEDERREITER:
            zeta = niederreiter31(seed);
            xi[0] = (double) zeta[0] * RECIP;
            xi[1] = (double) zeta[1] * RECIP;
            xi[2] = (double) zeta[2] * RECIP;
            xi[3] = (double) zeta[3] * RECIP;
            break;
        default:
            logFatal(-1, "sample4D", "QMC Sequence %s not yet implemented", SEQ4D_NAME(seq));
    }

    return xi;
}

/**
The following routines are safe with Sample4D(), which calls only
31-bit sequences (including 31-bit Niederreiter sequence). If
you are looking for such a routine to use directly in conjunction
with the routine Nied() or NextNiedInRange(), you should use
the foldSample() routine in Niederreiter.h instead.
Nied() and NextNiedInRange() are 63-bit unless compiled without
'unsigned long long' support
*/
void
foldSampleU(unsigned *xi1, unsigned *xi2) {
    foldSample31(xi1, xi2);
}

void
foldSampleF(double *xi1, double *xi2) {
    unsigned zeta1 = (unsigned)(*xi1 * RECIP1);
    unsigned zeta2 = (unsigned)(*xi2 * RECIP1);
    foldSampleU(&zeta1, &zeta2);
    *xi1 = (double) zeta1 * RECIP;
    *xi2 = (double) zeta2 * RECIP;
}
