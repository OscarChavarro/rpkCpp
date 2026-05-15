#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Faure's quasiMonteCarlo sequences
*/

#include "java/lang/Math.h"
#include "numericalAnalysis/quasiMonteCarlo/Faure.h"
#include "numericalAnalysis/quasiMonteCarlo/FaureSequenceLimits.h"

int Faure::prime[FaureSequenceLimits::MAX_DIMENSION] = FAURE_PRIME_DATA;
int Faure::ix[FaureSequenceLimits::MAX_DIMENSION][FaureSequenceLimits::MAX_PRIME_DIGITS] = {{0}};  // PR part representation of x
int Faure::dimension = 0;
int Faure::primeBase = 0;

// diameter[s] is 1e diameter > s
int Faure::nextN = 0;
int Faure::skip = 0;
int Faure::nDigits = 0;
int Faure::generatorMatrix[FaureSequenceLimits::MAX_DIMENSION][FaureSequenceLimits::MAX_PRIME_DIGITS][FaureSequenceLimits::MAX_PRIME_DIGITS] = {{{0}}};

int
Faure::setFaureC() {
    // First set up generatorMatrix[0][][] (transposed Pascal matrix)
    for ( int j = 0; j < nDigits; j++ ) {
        for ( int k = j; k < nDigits; k++ ) {
            if ( j == 0 || j == k ) {
                generatorMatrix[0][j][k] = 1;
            } else {
                generatorMatrix[0][j][k] = (generatorMatrix[0][j][k - 1] + generatorMatrix[0][j - 1][k - 1]) % primeBase;
            }
        }
    }

    // Use generatorMatrix[0][][] to compose generatorMatrix[i][][]
    // generatorMatrix[0] is overwritten if i=0 -> becomes unit matrix
    for ( int i = dimension - 1; i >= 0; i-- ) {
        for ( int j = 0; j < nDigits; j++ ) {
            for ( int k = j; k < nDigits; k++ ) {
                generatorMatrix[i][j][k] = (generatorMatrix[0][j][k] * ((int)(Math::pow(((float)(i)), ((float)(k - j)))))) % primeBase;
            }
        }
    }

    return 0;
}

int
Faure::setGFaureC() {
    unsigned P[FaureSequenceLimits::MAX_PRIME_DIGITS][FaureSequenceLimits::MAX_PRIME_DIGITS];

    // Pascal matrix
    for ( int j = 0; j < nDigits; j++ ) {
        P[j][0] = 1;
        P[j][j] = 1;
    }

    for ( int j = 1; j < nDigits; j++ ) {
        for ( int k = 1; k < j; k++ ) {
            P[j][k] = (P[j - 1][k - 1] + P[j - 1][k]) % primeBase;
        }
        for ( int k = j + 1; k < nDigits; k++ ) {
            P[j][k] = 0;
        }
    }

    // [Tezuka95, p179-180]
    for ( int i = 0; i < dimension; i++ ) {
        // Compute generatorMatrix[i]
        for ( int m = 0; m < nDigits; m++ ) {
            for ( int n = 0; n < nDigits; n++ ) {
                int Q = m < n ? m : n;
                generatorMatrix[i][m][n] = 0;
                for ( int q = 0; q <= Q; q++ ) {
                    generatorMatrix[i][m][n] = ((int)(generatorMatrix[i][m][n] + P[m][q] * P[n][q] * ((int)(Math::pow(((float)(i)), ((float)(m + n - 2 * q))))))) % primeBase;
                }
            }
        }
    }

    return 0;
}

/**
If NO_GRAY is defined, you can't mix NextFaure() and Faure() calls,
but faure() will be faster because it doesn't need to convert to
seed to it's Gray code
*/
double *
Faure::nextFaure() {
    int save;
    static double x[FaureSequenceLimits::MAX_DIMENSION];
    double xx;

    save = nextN;
    int k = 1;
    while ( (save % primeBase) == (primeBase - 1) ) {
        k = k + 1;
        save = save / primeBase;
    }
    for ( int i = 0; i < dimension && i < FaureSequenceLimits::MAX_DIMENSION; i++ ) {
        xx = 0;
        for ( int j = nDigits - 1; j >= 0; j-- ) {
            if ( j < FaureSequenceLimits::MAX_PRIME_DIGITS ) {
                ix[i][j] = (ix[i][j] + generatorMatrix[i][j][k - 1]) % primeBase;
                xx = xx / primeBase + ix[i][j];
            }
        }
        x[i] = xx / primeBase;
    }
    nextN += 1;
    return x;
}

/**
Return sample with given index
*/
double *
Faure::faure(int seed) {
    int save;
    static double x[FaureSequenceLimits::MAX_DIMENSION];
    double xx;

    nextN = seed + skip + 1;
    for ( int i = 0; i < dimension; i++ ) {
        xx = 0;
        for ( int j = nDigits - 1; j >= 0; j-- ) {
            save = nextN;
            ix[i][j] = 0;
            for ( int k = 0; k < nDigits; k++ ) {
                ix[i][j] = (ix[i][j] + generatorMatrix[i][j][k] * save) % primeBase;
                save /= primeBase;
            }
            xx = xx / primeBase + ix[i][j];
        }
        x[i] = xx / primeBase;
    }
    return x;
}

/**
Initialize for Original Faure sequence
*/
void
Faure::initOriginalFaureSequence(int iDim) {
    dimension = iDim;
    nextN = 0;
    primeBase = prime[dimension - 1];
    nDigits = ((int)(Math::log(((double)(FaureSequenceLimits::MAX_SEED))) / Math::log(((double)(primeBase))) + 1));
    Faure::setFaureC();
    for ( int i = 0; i < dimension; i++ ) {
        for ( int j = 0; j < nDigits; j++ ) {
            ix[i][j] = 0;
        }
    }

    skip = ((int)(Math::pow(((float)(primeBase)), 4.0f))) - 1;
    for ( int i = 1; i <= skip; i++ ) {
        // Warm up
        Faure::nextFaure();
    }
}

/**
Initialize for generalized Faure sequence
*/
void
Faure::initGeneralizedFaureSequence(int iDim) {
    dimension = iDim;
    nextN = 0;
    primeBase = prime[dimension - 1];
    nDigits = ((int)(Math::log(((double)(FaureSequenceLimits::MAX_SEED))) / Math::log(((double)(primeBase))) + 1));
    Faure::setGFaureC();
    for ( int i = 0; i < dimension; i++ ) {
        for ( int j = 0; j < nDigits; j++ ) {
            ix[i][j] = 0;
        }
    }

    skip = ((int)(Math::pow(((float)(primeBase)), 4.0f) - 1));
    for ( int i = 1; i <= skip; i++ ) {
        // Warm up
        Faure::nextFaure();
    }
}
