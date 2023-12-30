/* scrambledhalton.c: scrambled halton QMC sample sequence */

#include "QMC/scrambledhalton.h"

#define MAXDIM 10

double *ScrambledHalton(unsigned nextn, int dim) {
    int i, j, a, b, m;
    static int priem[MAXDIM] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    static double sample[MAXDIM];
    double fi, bi, bp;

    for ( i = 0; i < dim; i++ ) {
        b = priem[i];  /* elke component gebruikt andere basis */
        bi = 1.0 / b;

        fi = 0;
        bp = 1;
        m = 0;
        for ( j = nextn; j > 0; j /= b ) {
            bp = bp * bi;
            a = j % b;  /* a is m-de cijfer uit b-aire voorstelling van nextn */
            a = (a + m) % b; /* permutatie van a, methode van  Warnock */
            fi = fi + a * bp;
            m += 1;
        }
        sample[i] = fi;
    }
    nextn += 1;
    return sample;
}
