/**
Macro's to manipulate radiance coefficients
*/

#ifndef __COEFFICIENTS__
#define __COEFFICIENTS__

#include "material/color.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

#define CLEARCOEFFICIENTS(c, n) {int _i; COLOR *_c; \
  for (_i=0, _c=(c); _i<(n); _i++, _c++) colorClear(*_c); \
}

#define COPYCOEFFICIENTS(dst, src, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(src); _i<(n); _i++, _d++, _s++) *_d=*_s; \
}

#define ADDCOEFFICIENTS(dst, extra, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(extra); _i<(n); _i++, _d++, _s++) { \
    colorAdd(*_d, *_s, *_d); \
}}

#endif
