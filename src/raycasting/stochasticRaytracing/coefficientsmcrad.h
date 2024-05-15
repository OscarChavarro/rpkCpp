#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

inline void
stochasticRadiosityClearCoefficients(ColorRgb *c, const GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        c[i].clear();
    }
}

inline void
stochasticRadiosityCopyCoefficients(ColorRgb *dst, const ColorRgb *src, const GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;
    const ColorRgb *s;

    for ( i = 0, d = dst, s = src; i < galerkinBasis->size; i++, d++, s++ ) {
        *d = *s;
    }
}

inline void
stochasticRadiosityAddCoefficients(ColorRgb *dst, const ColorRgb *extra, const GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;
    const ColorRgb *s;

    for ( i = 0, d = dst, s = extra; i < galerkinBasis->size; i++, d++, s++ ) {
        d->add(*d, *s);
    }
}

inline void
stochasticRadiosityScaleCoefficients(float scale, ColorRgb *color, const GalerkinBasis *galerkinBasis) {
    int i;
    ColorRgb *d;

    for ( i = 0, d = color; i < galerkinBasis->size; i++, d++ ) {
        d->scale(scale);
    }
}

inline void
stochasticRadiosityMultiplyCoefficients(const ColorRgb &color, ColorRgb *coefficients, const GalerkinBasis *galerkinBasis) {
    ColorRgb *d;
    ColorRgb c = color;

    int i;
    for ( i = 0, d = coefficients; i < galerkinBasis->size; i++, d++) {
        d->selfScalarProduct(c);
    }
}

extern void initCoefficients(StochasticRadiosityElement *elem);
extern void disposeCoefficients(StochasticRadiosityElement *elem);
extern void allocCoefficients(StochasticRadiosityElement *elem);
extern void reAllocCoefficients(StochasticRadiosityElement *elem);

#endif
