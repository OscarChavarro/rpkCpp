#ifndef COEFFICIENTS__
#define COEFFICIENTS__

#include "vsdk/toolkit/common/color/ColorRgbMath.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Coefficientsmcrad final {
  public:
    static inline void stochasticRadiosityClearCoefficients(ColorRgb *c, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            c[i] = ColorRgb(0.0, 0.0, 0.0);
        }
    }

    static inline void stochasticRadiosityCopyCoefficients(ColorRgb *dst, const ColorRgb *src, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            dst[i] = src[i];
        }
    }

    static inline void stochasticRadiosityAddCoefficients(ColorRgb *dst, const ColorRgb *extra, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            dst[i] = dst[i].add(extra[i]);
        }
    }

    static inline void stochasticRadiosityScaleCoefficients(float scale, ColorRgb *color, const GalerkinBasis *galerkinBasis) {
        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            color[i] = color[i].multiply(scale);
        }
    }

    static inline void stochasticRadiosityMultiplyCoefficients(const ColorRgb &color, ColorRgb *coefficients, const GalerkinBasis *galerkinBasis) {
        ColorRgb c = color;

        for ( int i = 0; i < galerkinBasis->size; i++ ) {
            coefficients[i] = ColorRgbMath::product(coefficients[i], c);
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
