#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"

class StochasticRadiosityElement;

inline void
stochasticRadiosityClearCoefficients(ColorRgb *c, const GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        c[i].clear();
    }
}

inline void
stochasticRadiosityCopyCoefficients(ColorRgb *dst, const ColorRgb *src, const GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        dst[i] = src[i];
    }
}

inline void
stochasticRadiosityAddCoefficients(ColorRgb *dst, const ColorRgb *extra, const GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        dst[i].add(dst[i], extra[i]);
    }
}

inline void
stochasticRadiosityScaleCoefficients(float scale, ColorRgb *color, const GalerkinBasis *galerkinBasis) {
    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        color[i].scale(scale);
    }
}

inline void
stochasticRadiosityMultiplyCoefficients(const ColorRgb &color, ColorRgb *coefficients, const GalerkinBasis *galerkinBasis) {
    ColorRgb c = color;

    for ( int i = 0; i < galerkinBasis->size; i++ ) {
        coefficients[i].selfScalarProduct(c);
    }
}

extern void initCoefficients(StochasticRadiosityElement *elem);
extern void disposeCoefficients(StochasticRadiosityElement *elem);
extern void allocCoefficients(StochasticRadiosityElement *elem);
extern void reAllocCoefficients(StochasticRadiosityElement *elem);

#endif
