/**
Sobol QMC sequence
*/

#include "common/mymath.h"
#include "QMC/sobol.h"

#define MAXDIM 5
#define VMAX 30

static int dim;
static int nextn;
static int x[MAXDIM];
static int v[MAXDIM][VMAX];
static int skip;
static double RECIPD;

double *
nextSobol() {
    static double xx[MAXDIM];
    int c;
    int i;
    int save;

    c = 1;
    save = nextn;
    while ((save % 2) == 1 ) {
        c += 1;
        save = save / 2;
    }
    for ( i = 0; i < dim; i++ ) {
        x[i] = x[i] ^ (v[i][c - 1] << (VMAX - c));
        xx[i] = x[i] * RECIPD;
    }
    nextn += 1;

    return xx;
}

// Translate n into Gray code
#define GRAY(n) (n ^ (n>>1))

double *
sobol(int seed) {
    static double xx[MAXDIM];
    int c;
    int gray;

    seed += skip + 1;
    for ( int i = 0; i < dim; i++ ) {
        x[i] = 0;
        c = 1;
        gray = GRAY(seed);
        while ( gray ) {
            if ( gray & 1 ) {
                x[i] = x[i] ^ (v[i][c - 1] << (VMAX - c));
            }
            c++;
            gray >>= 1;
        }

        xx[i] = x[i] * RECIPD;
    }

    return xx;
}

void
initSobol(int idim) {
    int i;
    int j;
    int k;
    int m;
    int save;
    int d[MAXDIM];
    int POLY[MAXDIM];

    nextn = 0;
    dim = idim;
    RECIPD = 1. / pow(2.0, VMAX);

    // Primitieve veeltermen inlezen
    POLY[0] = 3;
    d[0] = 1;  // x + 1
    POLY[1] = 7;
    d[1] = 2;  // x^2 + x + 1
    POLY[2] = 11;
    d[2] = 3;  // x^3 + x + 1
    POLY[3] = 19;
    d[3] = 4;  // x^4 + x  + 1
    POLY[4] = 37;
    d[4] = 5;  // x^5 + x^2 + 1

    // beginwaarden v inlezen
    // alle beginwaarden 1 --> begin van sequentie waardeloos!
    for ( i = 0; i < dim; i++ ) {
        for ( j = 0; j < d[i]; j++ ) {
            v[i][j] = 1;
        }
    }

    // Rest van v berekenen
    for ( i = 0; i < dim; i++ ) {
        for ( j = d[i]; j < VMAX; j++ ) {
            v[i][j] = v[i][j - d[i]];
            save = POLY[i];
            m = (int)std::pow(2, d[i]);
            for ( k = d[i]; k > 0; k-- ) {
                v[i][j] = v[i][j] ^ m * (save % 2) * v[i][j - k];
                save = save / 2;
                m = m / 2;
            }
        }
    }

    for ( i = 0; i < dim; i++ ) {
        x[i] = 0;
    }
    skip = (int)std::pow(2, 6); // niet deterministisch!
    for ( i = 1; i <= skip; i++ ) {
        nextSobol();
    }
    // Begin van sequentie weggooien want gelijke beginwaarden
}
