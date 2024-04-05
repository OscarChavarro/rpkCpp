#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

inline void
stochasticRadiosityClearCoefficients(ColorRgb *c, GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        c[i].clear();
    }
}

inline void
stochasticRadiosityCopyCoefficients(ColorRgb *dst, ColorRgb *src, GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;
    ColorRgb *s;

    for ( i = 0, d = dst, s = src; i < galerkinBasis->size; i++, d++, s++ ) {
        *d = *s;
    }
}

inline void
stochasticRadiosityAddCoefficients(ColorRgb *dst, ColorRgb *extra, GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;
    ColorRgb *s;

    for ( i = 0, d = dst, s = extra; i < galerkinBasis->size; i++, d++, s++ ) {
        colorAdd(*d, *s, *d);
    }
}

inline void
stochasticRadiosityScaleCoefficients(float scale, ColorRgb *color, GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;

    for ( i = 0, d = color; i < galerkinBasis->size; i++, d++ ) {
        d->scale(scale);
    }
}

inline void
stochasticRadiosityMultiplyCoefficients(ColorRgb &color, ColorRgb *coefficients, GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;
    ColorRgb c = color;

    for ( i = 0, d = coefficients; i < galerkinBasis->size; i++, d++) {
        d->selfScalarProduct(c);
    }
}

extern void initCoefficients(StochasticRadiosityElement *elem);
extern void disposeCoefficients(StochasticRadiosityElement *elem);
extern void allocCoefficients(StochasticRadiosityElement *elem);
extern void reAllocCoefficients(StochasticRadiosityElement *elem);

#endif
