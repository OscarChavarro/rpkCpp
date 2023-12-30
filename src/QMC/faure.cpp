/*
Faure's QMC sequences
*/

#include "common/mymath.h"
#include "QMC/faure.h"

#define MAXDIM 10
#define PRDIM 30
#define MAXSEED 2147483647

static int ix[MAXDIM][PRDIM];  /* PR-delige voorstelling van x */
static int dim, PR, priem[MAXDIM] = {2, 3, 5, 5, 7, 7, 11, 11, 11, 11};
/* priem[s] is 1e priemgetal >s*/
static int nextn, skip, ndigits;
static int C[MAXDIM][PRDIM][PRDIM];  /* generator matrix */

static int setFaureC() {
    int i, j, k;

    /* eerst C[0][][] opstellen (getransponeerde Pascal matrix) */
    for ( j = 0; j < ndigits; j++ ) {
        for ( k = j; k < ndigits; k++ ) {
            if ( j == 0 ) {
                C[0][j][k] = 1;
            } else if ( j == k ) {
                C[0][j][k] = 1;
            } else {
                C[0][j][k] = (C[0][j][k - 1] + C[0][j - 1][k - 1]) % PR;
            }
        }
    }

    /* C[0][][] gebruiken om C[i][][] op te stellen.
     * C[0] wordt overschreven als i=0 ====> wordt eenheidsmatrix */
    for ( i = dim - 1; i >= 0; i-- ) {
        for ( j = 0; j < ndigits; j++ ) {
            for ( k = j; k < ndigits; k++ ) {
                C[i][j][k] = (C[0][j][k] * (int) pow(i, k - j)) % PR;
            }
        }
    }

    return 0;
}

static int setGFaureC() {
    int i, j, k;
    unsigned P[PRDIM][PRDIM];

    /* Pascal matrix */
    for ( j = 0; j < ndigits; j++ ) {
        P[j][0] = 1;
        P[j][j] = 1;
    }
    for ( j = 1; j < ndigits; j++ ) {
        for ( k = 1; k < j; k++ ) {
            P[j][k] = (P[j - 1][k - 1] + P[j - 1][k]) % PR;
        }
        for ( k = j + 1; k < ndigits; k++ ) {
            P[j][k] = 0;
        }
    }

    /* [Tezuka95, p179-180] */
    for ( i = 0; i < dim; i++ ) {
        /* compute C[i] */
        int m, j;
        for ( m = 0; m < ndigits; m++ ) {
            for ( j = 0; j < ndigits; j++ ) {
                int q, Q = m < j ? m : j;
                C[i][m][j] = 0;
                for ( q = 0; q <= Q; q++ ) {
                    C[i][m][j] = (C[i][m][j] + P[m][q] * P[j][q] * (int) pow(i, m + j - 2 * q)) % PR;
                }
            }
        }
    }

    return 0;
}

double *NextFaure() {
    int i, j, k, save;
    static double x[MAXDIM];
    double xx;

    save = nextn;
    k = 1;
    while ((save % PR) == (PR - 1)) {
        k = k + 1;
        save = save / PR;
    }
    for ( i = 0; i < dim; i++ ) {
        xx = 0;
        for ( j = ndigits - 1; j >= 0; j-- ) {
            ix[i][j] = (ix[i][j] + C[i][j][k - 1]) % PR;
            xx = xx / PR + ix[i][j];
        }
        x[i] = xx / PR;
    }
    nextn += 1;
    return x;
}

double *Faure(int seed) {
    int i, j, k, save;
    static double x[MAXDIM];
    double xx;

    nextn = seed + skip + 1;
    for ( i = 0; i < dim; i++ ) {
        xx = 0;
        for ( j = ndigits - 1; j >= 0; j-- ) {
            save = nextn;
            ix[i][j] = 0;
            for ( k = 0; k < ndigits; k++ ) {
                ix[i][j] = (ix[i][j] + C[i][j][k] * save) % PR;
                save /= PR;
            }
            xx = xx / PR + ix[i][j];
        }
        x[i] = xx / PR;
    }
    return x;
}

void InitFaure(int idim) {
    int i, j;

    dim = idim;
    nextn = 0;
    PR = priem[dim - 1];
    ndigits = log((double) MAXSEED) / log((double) PR) + 1;
    setFaureC();
    for ( i = 0; i < dim; i++ ) {
        for ( j = 0; j < ndigits; j++ ) {
            ix[i][j] = 0;
        }
    }

    skip = pow(PR, 4) - 1;
    for ( i = 1; i <= skip; i++ ) {
        NextFaure();
    } /* warm-up */
}

void InitGFaure(int idim) {
    int i, j;

    dim = idim;
    nextn = 0;
    PR = priem[dim - 1];
    ndigits = log((double) MAXSEED) / log((double) PR) + 1;
    setGFaureC();
    for ( i = 0; i < dim; i++ ) {
        for ( j = 0; j < ndigits; j++ ) {
            ix[i][j] = 0;
        }
    }

    skip = pow(PR, 4) - 1;
    for ( i = 1; i <= skip; i++ ) {
        NextFaure();
    } /* warm-up */
}

