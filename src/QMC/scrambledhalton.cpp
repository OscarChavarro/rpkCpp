/**
Scrambled halton QMC sample sequence
*/

#include "QMC/scrambledhalton.h"

#define MAX_DIM 10

double *
scrambledHalton(unsigned nextN, int dim) {
    int i;
    int j;
    int a;
    int b;
    int m;
    static int prime[MAX_DIM] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    static double sample[MAX_DIM];
    double fi;
    double bi;
    double bp;

    for ( i = 0; i < dim; i++ ) {
        b = prime[i]; // Each component uses different base
        bi = 1.0 / b;

        fi = 0;
        bp = 1;
        m = 0;
        for ( j = (int)nextN; j > 0; j /= b ) {
            bp = bp * bi;
            a = j % b; // Variable "a" is m-th digit from b-ary representation of nextN
            a = (a + m) % b; // Permutation of variable "a", Warnock's method
            fi = fi + a * bp;
            m += 1;
        }
        sample[i] = fi;
    }
    return sample;
}
