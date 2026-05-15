#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "common/color/ColorRgbMutable.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Coefficientsmcrad{ public:
    static inline void stchsRadClearCoeff(ColorRgbMutable *c, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ c[i].clear();
        }
    }

    static inline void stchsRadCopyCoeff(ColorRgbMutable *dst, const ColorRgbMutable *src, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ dst[i] = src[i];
        }
    }

    static inline void stchsRadAddCoeff(ColorRgbMutable *dst, const ColorRgbMutable *extra, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ dst[i].add(dst[i], extra[i]);
        }
    }

    static inline void stchsRadScaleCoeff(float scale, ColorRgbMutable *color, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ color[i].scale(scale);
        }
    }

    static inline void stchsRadMltplCoeff(const ColorRgbMutable &color, ColorRgbMutable *coefficients, const GalerkinBasis *galerkinBasis){ ColorRgbMutable c = color;

        for ( int i = 0; i < galerkinBasis->size; i++){ coefficients[i].selfScalarProduct(c);
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
