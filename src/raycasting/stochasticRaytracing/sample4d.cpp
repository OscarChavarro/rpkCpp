/**
4D vector sampling
*/

#include <cstdlib>

#include "common/error.h"
#include "QMC/halton.h"
#include "QMC/scrambledhalton.h"
#include "QMC/sobol.h"
#include "QMC/faure.h"
#include "QMC/nied31.h"
#include "raycasting/stochasticRaytracing/sample4d.h"

static SEQ4D seq = S4D_RANDOM;

/**
Also initialises the sequence
*/
void
setSequence4D(SEQ4D sequence) {
    seq = sequence;
    switch ( seq ) {
        case S4D_SOBOL:
            initSobol(4);
            break;
        case S4D_FAURE:
            InitFaure(4);
            break;
        case S4D_GFAURE:
            InitGFaure(4);
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
    unsigned *zeta;
    double *xx;

    switch ( seq ) {
        case S4D_RANDOM:
            xi[0] = drand48();
            xi[1] = drand48();
            xi[2] = drand48();
            xi[3] = drand48();
            break;
        case S4D_HALTON:
            xi[0] = Halton2(seed);
            xi[1] = Halton3(seed);
            xi[2] = Halton5(seed);
            xi[3] = Halton7(seed);
            break;
        case S4D_SCRAMHALTON:
            xx = ScrambledHalton(seed, 4);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case S4D_SOBOL:
            xx = sobol(seed);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case S4D_FAURE:
        case S4D_GFAURE:
            xx = Faure(seed);
            xi[0] = xx[0];
            xi[1] = xx[1];
            xi[2] = xx[2];
            xi[3] = xx[3];
            break;
        case S4D_NIEDERREITER:
            zeta = Nied31(seed);
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
with the routined Nied() or NextNiedInRange(), you should use
the foldSample() routine in niederreiter.h instead.
Nied() and NextNiedInRange() are 63-bit unless compiled without
'unsigned long long' support
*/
void
foldSampleU(unsigned *xi1, unsigned *xi2) {
    foldSample31(xi1, xi2);  /* declared in nied31.h */
}

void
foldSampleF(double *xi1, double *xi2) {
    unsigned zeta1 = (*xi1 * RECIP1);
    unsigned zeta2 = (*xi2 * RECIP1);
    foldSampleU(&zeta1, &zeta2);
    *xi1 = (double) zeta1 * RECIP;
    *xi2 = (double) zeta2 * RECIP;
}
