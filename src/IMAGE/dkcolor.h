/**
Routines using pixel color values.
     12/31/85

Two color representations are used, one for calculation and
another for storage.  Calculation is done with three floats
for speed.  Stored color values use 4 bytes which contain
three single byte mantissas and a common exponent.
*/

#include <cstdio>
#include "common/ColorRgb.h"

#define RED 0
#define GREEN 1
#define BLUE 2

// Exponent same for either format
#define EXP 3

// Excess used for exponent
#define COL_XS 128

typedef unsigned char BYTE; // 8-bit unsigned integer

typedef BYTE COLR[4]; // Red, green, blue (or X,Y,Z), exponent

typedef float COLOR[3]; // Red, green, blue (or X,Y,Z)

typedef float COLOR_MATRIX[3][3]; // Color coordinate conversion matrix

#define CIE_x_r 0.640 // Nominal CRT primaries
#define CIE_y_r 0.330
#define CIE_x_g 0.290
#define CIE_y_g 0.600
#define CIE_x_b 0.150
#define CIE_y_b 0.060
#define CIE_x_w 0.3333 // Use true white
#define CIE_y_w 0.3333

#define CIE_D ( CIE_x_r * (CIE_y_g - CIE_y_b) + CIE_x_g * (CIE_y_b - CIE_y_r) + CIE_x_b * (CIE_y_r - CIE_y_g)
#define CIE_C_rD ( (1.0/CIE_y_w) * ( CIE_x_w*(CIE_y_g - CIE_y_b) - CIE_y_w*(CIE_x_g - CIE_x_b) + CIE_x_g*CIE_y_b - CIE_x_b*CIE_y_g) )
#define CIE_C_gD ( (1.0/CIE_y_w) * ( CIE_x_w*(CIE_y_b - CIE_y_r) - CIE_y_w*(CIE_x_b - CIE_x_r) - CIE_x_r*CIE_y_b + CIE_x_b*CIE_y_r) )
#define CIE_C_bD ( (1.0/CIE_y_w) * ( CIE_x_w*(CIE_y_r - CIE_y_g) - CIE_y_w*(CIE_x_r - CIE_x_g) + CIE_x_r*CIE_y_g - CIE_x_g*CIE_y_r) )

#define CIE_rf (CIE_y_r*CIE_C_rD/CIE_D)
#define CIE_gf (CIE_y_g*CIE_C_gD/CIE_D)
#define CIE_bf (CIE_y_b*CIE_C_bD/CIE_D)

/* As of 9-94, CIE_rf=.265074126, CIE_gf=.670114631 and CIE_bf=.064811243 */

/***** The following definitions are valid for RGB colors only *****/

/**
 * Conversions to and from XYZ space generally don't apply WHITE_EFFICACY.
 * If you need Y to be luminance (cd/m^2), this must be applied when
 * converting from radiance (watts/sr/m^2).
 */

extern COLOR_MATRIX GLOBAL_rgb2XyzMat; // RGB to XYZ conversion matrix
extern COLOR_MATRIX GLOBAL_xyz2RgbMat; // XYZ to RGB conversion matrix

char *dkColorTempBuffer(unsigned int len);
int dkColorWriteColrs(COLR *scanline, int len, FILE *fp);
int dkColorWriteScan(ColorRgb *scanline, int len, FILE *fp);
void dkColorSetColr(COLR clr, double r, double g, double b);
