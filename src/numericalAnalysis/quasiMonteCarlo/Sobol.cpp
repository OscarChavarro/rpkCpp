#include "java/lang/Math.h"
#include "numericalAnalysis/quasiMonteCarlo/Sobol.h"

int Sobol::dim = 0;
int Sobol::nextN = 0;
int Sobol::x[Sobol::MAX_DIM] = {};
int Sobol::v[Sobol::MAX_DIM][Sobol::V_MAX] = {};
int Sobol::skip = 0;
double Sobol::recip = 0.0;

double *
Sobol::nextSobol() {
    static double xx[MAX_DIM];

    int c = 1;
    int save = nextN;
    while ( (save % 2) == 1 ) {
        c += 1;
        save = save / 2;
    }
    for ( int i = 0; i < dim; i++ ) {
        x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
        xx[i] = x[i] * recip;
    }
    nextN += 1;

    return xx;
}

// Translate n into Gray code
int
Sobol::sobolGray(int n) {
    return n ^ (n >> 1);
}

double *
Sobol::sobol(int seed) {
    static double xx[MAX_DIM];

    seed += skip + 1;
    for ( int i = 0; i < dim; i++ ) {
        x[i] = 0;
        int c = 1;
        int gray = Sobol::sobolGray(seed);
        while ( gray ) {
            if ( gray & 1 ) {
                x[i] = x[i] ^ (v[i][c - 1] << (V_MAX - c));
            }
            c++;
            gray >>= 1;
        }

        xx[i] = x[i] * recip;
    }

    return xx;
}

void
Sobol::initSobol(int iDim) {
    int d[MAX_DIM];
    int POLY[MAX_DIM];

    nextN = 0;
    dim = iDim;
    recip = 1.0 / java::Math::pow(2.0, V_MAX);

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
            int m = static_cast<int>(java::Math::pow(2.0f, static_cast<float>(d[i])));
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
    skip = static_cast<int>(java::Math::pow(2.0f, 6.0f)); // Not deterministic!
    for ( int i = 1; i <= skip; i++ ) {
        // Discard the beginning of the sequence because the initial values are the same
        Sobol::nextSobol();
    }
}
