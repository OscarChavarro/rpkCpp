/**
Halton quasi Monte Carlo sample generator
*/

#include "QMC/halton.h"

double
Halton2(int i) {
    unsigned long h, f;

    h = i & 1;
    f = 2;
    i >>= 1;
    while ( i ) {
        h <<= 1;
        h += (i & 1);
        i >>= 1;
        f <<= 1;
        h <<= 1;
        h += (i & 1);
        i >>= 1;
        f <<= 1;
        h <<= 1;
        h += (i & 1);
        i >>= 1;
        f <<= 1;
        h <<= 1;
        h += (i & 1);
        i >>= 1;
        f <<= 1;
    }

    return (double) h / (double) f;
}

double
Halton3(int i) {
    unsigned long h, f;
    unsigned long j = i;
    i /= 3;
    h = j - ((i << 1) + i);
    f = 3;
    while ( i > 0 ) {
        unsigned long k;
        j = i;
        i /= 3;
        k = h - i;
        h = j + (k << 1) + k;
        f = (f << 1) + f;
    }

    return (double) h / (double) f;
}

double
Halton5(int i) {
    unsigned long h, f;
    unsigned long j = i;
    i /= 5;
    h = j - ((i << 2) + i);
    f = 5;
    while ( i > 0 ) {
        unsigned long k;
        j = i;
        i /= 5;
        k = h - i;
        h = j + (k << 2) + k;
        f = (f << 2) + f;
    }

    return (double) h / (double) f;
}

double
Halton7(int i) {
    unsigned long h, f;
    unsigned long j = i;
    i /= 7;
    h = j - ((i << 2) + (i << 1) + i);
    f = 7;
    while ( i > 0 ) {
        unsigned long k;
        j = i;
        i /= 7;
        k = h - i;
        h = j + (k << 2) + (k << 1) + k;
        f = (f << 2) + (f << 1) + f;
    }

    return (double) h / (double) f;
}
