/* coefficients.h: macro's to manipulate radiance coefficients. */

#ifndef _COEFFICIENTS_H_
#define _COEFFICIENTS_H_

#include "material/color.h"

#define CLEARCOEFFICIENTS(c, n) {int _i; COLOR *_c; \
  for (_i=0, _c=(c); _i<(n); _i++, _c++) COLORCLEAR(*_c); \
}

#define COPYCOEFFICIENTS(dst, src, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(src); _i<(n); _i++, _d++, _s++) *_d=*_s; \
}

#define ADDCOEFFICIENTS(dst, extra, n) {int _i; COLOR *_d, *_s; \
  for (_i=0, _d=(dst), _s=(extra); _i<(n); _i++, _d++, _s++) { \
    COLORADD(*_d, *_s, *_d); \
}}

#define PRINTCOEFFICIENTS(fp, c, n) {int _i; \
  ColorPrint(fp, (c)[0]); \
  for (_i=1; _i<(n); _i++) { \
    fprintf(fp, ", "); \
    ColorPrint(fp, (c)[_i]); \
  }}

#endif
