/* coefficients.h: macro's to manipulate radiance coefficients. */

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

#define COPYCOEFFICIENTS(dst, src, galerkinBasis) { \
    int _i; \
    int _n = galerkinBasis->size; \
    COLOR *_d; \
    COLOR *_s; \
    for ( _i = 0, _d = (dst), _s = (src); _i < _n; _i++, _d++, _s++) { \
        *_d=*_s; \
    } \
}

#define ADDCOEFFICIENTS(dst, extra, galerkinBasis) { \
    int _i; \
    int _n = galerkinBasis->size; \
    COLOR *_d; \
    COLOR *_s; \
    for ( _i = 0, _d = (dst), _s = (extra); _i < _n; _i++, _d++, _s++) { \
        colorAdd(*_d, *_s, *_d); \
    } \
}

#define SCALECOEFFICIENTS(scale, color, galerkinBasis) { \
    int _i; \
    int _n = galerkinBasis->size; \
    COLOR *_d; \
    float _scale = (scale); \
    for ( _i = 0, _d = (color); _i < _n; _i++, _d++) { \
        colorScale(_scale, *_d, *_d); \
    } \
}

#define MULTCOEFFICIENTS(color, coeff, galerkinBasis) {int _i, _n=galerkinBasis->size; COLOR *_d; COLOR _col = (color); \
  for (_i=0, _d=(coeff); _i<_n; _i++, _d++) { \
    colorProduct(_col, *_d, *_d); \
}}

inline void
stochasticRaytracingPrintCoefficients(FILE *fp, COLOR *c, GalerkinBasis *galerkinBasis) {
    int _i;
    int _n = galerkinBasis->size;

    if ( _n > 0 ) {
        c[0].print(fp);
    }
    for (_i=1; _i<_n; _i++) {
        fprintf(fp, ", ");
        (c)[_i].print(fp);
    }
}

/* basically sets rad...  to nullptr */
extern void InitCoefficients(ELEMENT *elem);

/* disposes previously allocated coefficients */
extern void DisposeCoefficients(ELEMENT *elem);

/* allocates memory for radiance coefficients */
extern void AllocCoefficients(ELEMENT *elem);

/* re-allocates memory for radiance coefficients if
 * the currently desired approximation order is not the same
 * as the approximation order for which the element has
 * been initialised before. */
extern void ReAllocCoefficients(ELEMENT *elem);

#endif
