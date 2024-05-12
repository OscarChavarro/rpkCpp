/**
Faure's QMC sequences
*/

#include "java/lang/Math.h"
#include "QMC/faure.h"

#define MAX_DIM 10
#define PR_DIM 30
#define MAX_SEED 2147483647

static int globalIx[MAX_DIM][PR_DIM];  // PR part presentation of x
static int globalDim;
static int globalPR;
static int globalPrime[MAX_DIM] = {2, 3, 5, 5, 7, 7, 11, 11, 11, 11};

// diameter[s] is 1e diameter >s
static int globalNextN;
static int globalSkip;
static int globalNDigits;
static int C[MAX_DIM][PR_DIM][PR_DIM]; // generator matrix

static int
setFaureC() {
    // First set up C[0][][] (transposed Pascal matrix)
    for ( int j = 0; j < globalNDigits; j++ ) {
        for ( int k = j; k < globalNDigits; k++ ) {
            if ( j == 0 || j == k ) {
                C[0][j][k] = 1;
            } else {
                C[0][j][k] = (C[0][j][k - 1] + C[0][j - 1][k - 1]) % globalPR;
            }
        }
    }

    // Use C[0][][] to compose C[i][][]
    // C[0] is overwritten if i=0 -> becomes unit matrix
    for ( int i = globalDim - 1; i >= 0; i-- ) {
        for ( int j = 0; j < globalNDigits; j++ ) {
            for ( int k = j; k < globalNDigits; k++ ) {
                C[i][j][k] = (C[0][j][k] * (int)java::Math::pow((float)i, (float)(k - j))) % globalPR;
            }
        }
    }

    return 0;
}

static int
setGFaureC() {
    unsigned P[PR_DIM][PR_DIM];

    // Pascal matrix
    for ( int j = 0; j < globalNDigits; j++ ) {
        P[j][0] = 1;
        P[j][j] = 1;
    }

    for ( int j = 1; j < globalNDigits; j++ ) {
        for ( int k = 1; k < j; k++ ) {
            P[j][k] = (P[j - 1][k - 1] + P[j - 1][k]) % globalPR;
        }
        for ( int k = j + 1; k < globalNDigits; k++ ) {
            P[j][k] = 0;
        }
    }

    // [Tezuka95, p179-180]
    for ( int i = 0; i < globalDim; i++ ) {
        // Compute C[i]
        for ( int m = 0; m < globalNDigits; m++ ) {
            for ( int n = 0; n < globalNDigits; n++ ) {
                int Q = m < n ? m : n;
                C[i][m][n] = 0;
                for ( int q = 0; q <= Q; q++ ) {
                    C[i][m][n] = (int)(C[i][m][n] + P[m][q] * P[n][q] * (int)java::Math::pow((float)i, (float)(m + n - 2 * q))) % globalPR;
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
static double *
nextFaure() {
    int save;
    static double x[MAX_DIM];
    double xx;

    save = globalNextN;
    int k = 1;
    while ( (save % globalPR) == (globalPR - 1) ) {
        k = k + 1;
        save = save / globalPR;
    }
    for ( int i = 0; i < globalDim && i < MAX_DIM; i++ ) {
        xx = 0;
        for ( int j = globalNDigits - 1; j >= 0; j-- ) {
            if ( j < PR_DIM ) {
                globalIx[i][j] = (globalIx[i][j] + C[i][j][k - 1]) % globalPR;
                xx = xx / globalPR + globalIx[i][j];
            }
        }
        x[i] = xx / globalPR;
    }
    globalNextN += 1;
    return x;
}

/**
Return sample with given index
*/
double *
faure(int seed) {
    int save;
    static double x[MAX_DIM];
    double xx;

    globalNextN = seed + globalSkip + 1;
    for ( int i = 0; i < globalDim; i++ ) {
        xx = 0;
        for ( int j = globalNDigits - 1; j >= 0; j-- ) {
            save = globalNextN;
            globalIx[i][j] = 0;
            for ( int k = 0; k < globalNDigits; k++ ) {
                globalIx[i][j] = (globalIx[i][j] + C[i][j][k] * save) % globalPR;
                save /= globalPR;
            }
            xx = xx / globalPR + globalIx[i][j];
        }
        x[i] = xx / globalPR;
    }
    return x;
}

/**
Initialize for Original Faure sequence
*/
void
initFaure(int iDim) {
    globalDim = iDim;
    globalNextN = 0;
    globalPR = globalPrime[globalDim - 1];
    globalNDigits = (int)(std::log((double) MAX_SEED) / std::log((double) globalPR) + 1);
    setFaureC();
    for ( int i = 0; i < globalDim; i++ ) {
        for ( int j = 0; j < globalNDigits; j++ ) {
            globalIx[i][j] = 0;
        }
    }

    globalSkip = (int)java::Math::pow((float)globalPR, 4.0f) - 1;
    for ( int i = 1; i <= globalSkip; i++ ) {
        // Warm up
        nextFaure();
    }
}

/**
Initialize for generalized Faure sequence
*/
void
initGFaure(int iDim) {
    globalDim = iDim;
    globalNextN = 0;
    globalPR = globalPrime[globalDim - 1];
    globalNDigits = (int)(std::log((double) MAX_SEED) / std::log((double) globalPR) + 1);
    setGFaureC();
    for ( int i = 0; i < globalDim; i++ ) {
        for ( int j = 0; j < globalNDigits; j++ ) {
            globalIx[i][j] = 0;
        }
    }

    globalSkip = (int)(java::Math::pow((float)globalPR, 4.0f) - 1);
    for ( int i = 1; i <= globalSkip; i++ ) {
        // Warm up
        nextFaure();
    }
}
