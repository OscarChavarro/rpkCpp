#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "material/color.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

inline void
stochasticRadiosityClearCoefficients(COLOR *c, GalerkinBasis *galerkinBasis) {
    int i;
    for ( i = 0; i < galerkinBasis->size; i++ ) {
        colorClear(c[i]);
    }
}

inline void
stochasticRadiosityCopyCoefficients(COLOR *dst, COLOR *src, GalerkinBasis *galerkinBasis) {
    int i;
    COLOR *d;
    COLOR *s;

    for ( i = 0, d = dst, s = src; i < galerkinBasis->size; i++, d++, s++ ) {
        *d = *s;
    }
}

inline void
stochasticRadiosityAddCoefficients(COLOR *dst, COLOR *extra, GalerkinBasis *galerkinBasis) {
    int i;
    COLOR *d;
    COLOR *s;

    for ( i = 0, d = dst, s = extra; i < galerkinBasis->size; i++, d++, s++ ) {
        colorAdd(*d, *s, *d);
    }
}

inline void
stochasticRadiosityScaleCoefficients(float scale, COLOR *color, GalerkinBasis *galerkinBasis) {
    int i;
    COLOR *d;

    for ( i = 0, d = color; i < galerkinBasis->size; i++, d++ ) {
        colorScale(scale, *d, *d);
    }
}

inline void
stochasticRadiosityMultiplyCoefficients(COLOR &color, COLOR *coefficients, GalerkinBasis *galerkinBasis) {
    int i;
    COLOR *d;
    COLOR c = color;

    for ( i = 0, d = coefficients; i < galerkinBasis->size; i++, d++) {
        colorProduct(c, *d, *d);
    }
}

extern void stochasticRaytracingPrintCoefficients(FILE *fp, COLOR *c, GalerkinBasis *galerkinBasis);
extern void initCoefficients(StochasticRadiosityElement *elem);
extern void disposeCoefficients(StochasticRadiosityElement *elem);
extern void allocCoefficients(StochasticRadiosityElement *elem);
extern void reAllocCoefficients(StochasticRadiosityElement *elem);

#endif
