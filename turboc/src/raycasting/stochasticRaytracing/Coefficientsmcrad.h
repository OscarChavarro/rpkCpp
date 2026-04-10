#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "common/ColorRgb.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Coefficientsmcrad{ public:
    static inline void stchsRadClearCoeff(ColorRgb *c, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ c[i].clear();
        }
    }

    static inline void stchsRadCopyCoeff(ColorRgb *dst, const ColorRgb *src, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ dst[i] = src[i];
        }
    }

    static inline void stchsRadAddCoeff(ColorRgb *dst, const ColorRgb *extra, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ dst[i].add(dst[i], extra[i]);
        }
    }

    static inline void stchsRadScaleCoeff(float scale, ColorRgb *color, const GalerkinBasis *galerkinBasis){ for ( int i = 0; i < galerkinBasis->size; i++){ color[i].scale(scale);
        }
    }

    static inline void stchsRadMltplCoeff(const ColorRgb &color, ColorRgb *coefficients, const GalerkinBasis *galerkinBasis){ ColorRgb c = color;

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
