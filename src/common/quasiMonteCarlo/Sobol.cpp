#include "java/lang/Math.h"
#include "common/quasiMonteCarlo/Sobol.h"

static const int MAX_DIM = 5;
static const int V_MAX = 30;

static int dim;
static int nextN;
static int x[MAX_DIM];
static int v[MAX_DIM][V_MAX];
static int skip;
static double RECIP;

static double *
nextSobol() {
    static double xx[MAX_DIM];
    int c;
    int save;

    c = 1;
    save = nextN;
    while ( (save % 2) == 1 ) {
        c += 1;
        save = save / 2;
    }
    for ( int i = 0; i < dim; i++ ) {
        x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
        xx[i] = x[i] * RECIP;
    }
    nextN += 1;

    return xx;
}

// Translate n into Gray code
inline static int
sobolGray(int n) {
    return n ^ (n >> 1);
}

double *
sobol(int seed) {
    static double xx[MAX_DIM];
    int c;
    int gray;

    seed += skip + 1;
    for ( int i = 0; i < dim; i++ ) {
        x[i] = 0;
        c = 1;
        gray = sobolGray(seed);
        while ( gray ) {
            if ( gray & 1 ) {
                x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
            }
            c++;
            gray >>= 1;
        }

        xx[i] = x[i] * RECIP;
    }

    return xx;
}

void
initSobol(int iDim) {
    int d[MAX_DIM];
    int POLY[MAX_DIM];

    nextN = 0;
    dim = iDim;
    RECIP = 1.0 / java::Math::pow(2.0, V_MAX);

    // Reading primitive polynomials
    POLY[0] = 3;
    d[0] = 1; // x + 1
    POLY[1] = 7;
    d[1] = 2; // x^2 + x + 1
    POLY[2] = 11;
    d[2] = 3; // x^3 + x + 1
    POLY[3] = 19;
    d[3] = 4; // x^4 + x  + 1
    POLY[4] = 37;
    d[4] = 5; // x^5 + x^2 + 1

    // Initial values v read in all initial values 1 --> start of sequence worthless!
    for ( int i = 0; i < dim; i++ ) {
        for ( int j = 0; j < d[i]; j++ ) {
            v[i][j] = 1;
        }
    }

    // Calculate remainder of v
    for ( int i = 0; i < dim; i++ ) {
        for ( int j = d[i]; j < V_MAX; j++ ) {
            v[i][j] = v[i][j - d[i]];
            int save = POLY[i];
            int m = (int)java::Math::pow(2.0f, (float)d[i]);
            for ( int k = d[i]; k > 0; k-- ) {
                v[i][j] = v[i][j] ^ m * (save % 2) * v[i][j - k];
                save = save / 2;
                m = m / 2;
            }
        }
    }

    for ( int i = 0; i < dim; i++ ) {
        x[i] = 0;
    }
    skip = (int)java::Math::pow(2.0f, 6.0f); // Not deterministic!
    for ( int i = 1; i <= skip; i++ ) {
        // Discard the beginning of the sequence because the initial values are the same
        nextSobol();
    }
}
