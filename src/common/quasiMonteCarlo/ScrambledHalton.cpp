/**
Scrambled halton quasiMonteCarlo sample sequence
*/

#include "common/quasiMonteCarlo/ScrambledHalton.h"

static const int MAX_DIM = 10;

double *
scrambledHalton(unsigned nextN, int dim) {
    static int prime[MAX_DIM] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    static double sample[MAX_DIM];

    for ( int i = 0; i < dim; i++ ) {
        int b = prime[i]; // Each component uses different base
        double bi = 1.0 / b;
        double fi = 0;
        double bp = 1;
        int m = 0;

        for ( int j = (int)nextN; j > 0; j /= b ) {
            bp = bp * bi;
            int a = j % b; // Variable "a" is m-th digit from b-ary representation of nextN
            a = (a + m) % b; // Permutation of variable "a", Warnock's method
            fi = fi + a * bp;
            m += 1;
        }
        sample[i] = fi;
    }
    return sample;
}
