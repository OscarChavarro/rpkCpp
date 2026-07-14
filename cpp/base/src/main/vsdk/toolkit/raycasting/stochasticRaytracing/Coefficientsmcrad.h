#ifndef COEFFICIENTS__
#define COEFFICIENTS__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Coefficientsmcrad final {
  public:
    static inline void stochasticRadiosityClearCoefficients(ColorRgbMutable *c, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            c[i].clear();
        }
    }

    static inline void stochasticRadiosityCopyCoefficients(ColorRgbMutable *dst, const ColorRgbMutable *src, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            dst[i] = src[i];
        }
    }

    static inline void stochasticRadiosityAddCoefficients(ColorRgbMutable *dst, const ColorRgbMutable *extra, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            dst[i].add(dst[i], extra[i]);
        }
    }

    static inline void stochasticRadiosityScaleCoefficients(float scale, ColorRgbMutable *color, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            color[i].scale(scale);
        }
    }

    static inline void stochasticRadiosityMultiplyCoefficients(const ColorRgbMutable &color, ColorRgbMutable *coefficients, const GalerkinBasis *galerkinBasis) {
        ColorRgbMutable c = color;

        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            coefficients[i].selfScalarProduct(c);
        }
    }

    static void initCoefficients(StochasticRadiosityElement *elem);
    static void disposeCoefficients(StochasticRadiosityElement *elem);
    static void allocCoefficients(StochasticRadiosityElement *elem);
    static void reAllocCoefficients(StochasticRadiosityElement *elem);

  private:
    static GalerkinBasis *actualBasis(const StochasticRadiosityElement *elem);
};

#endif
