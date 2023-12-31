/*
 *  color.h - header for routines using pixel color values.
 *
 *     12/31/85
 *
 *  Two color representations are used, one for calculation and
 *  another for storage.  Calculation is done with three floats
 *  for speed.  Stored color values use 4 bytes which contain
 *  three single byte mantissas and a common exponent.
 */

#include <cstdio>

#define  RED_            0
#define  GRN            1
#define  BLU            2
#define  EXP            3       /* exponent same for either format */
#define  COLXS          128     /* excess used for exponent */

typedef unsigned char BYTE;    /* 8-bit unsigned integer */

typedef BYTE COLR[4];          /* red, green, blue (or X,Y,Z), exponent */

typedef float COLOR[3];        /* red, green, blue (or X,Y,Z) */

typedef float COLORMAT[3][3];  /* color coordinate conversion matrix */

#define  copycolr(c1, c2)        (c1[0]=c2[0],c1[1]=c2[1],                                 c1[2]=c2[2],c1[3]=c2[3])

#define  colval(col, pri)        ((col)[pri])

#define  copycolor(c1, c2)       ((c1)[0]=(c2)[0],(c1)[1]=(c2)[1],(c1)[2]=(c2)[2])

#define  CIE_x_r                0.640           /* nominal CRT primaries */
#define  CIE_y_r                0.330
#define  CIE_x_g                0.290
#define  CIE_y_g                0.600
#define  CIE_x_b                0.150
#define  CIE_y_b                0.060
#define  CIE_x_w                0.3333          /* use true white */
#define  CIE_y_w                0.3333

#define CIE_D           (       CIE_x_r*(CIE_y_g - CIE_y_b) +                                 CIE_x_g*(CIE_y_b - CIE_y_r) +                                 CIE_x_b*(CIE_y_r - CIE_y_g)     )
#define CIE_C_rD        ( (1./CIE_y_w) *                                 ( CIE_x_w*(CIE_y_g - CIE_y_b) -                                   CIE_y_w*(CIE_x_g - CIE_x_b) +                                   CIE_x_g*CIE_y_b - CIE_x_b*CIE_y_g     ) )
#define CIE_C_gD        ( (1./CIE_y_w) *                                 ( CIE_x_w*(CIE_y_b - CIE_y_r) -                                   CIE_y_w*(CIE_x_b - CIE_x_r) -                                   CIE_x_r*CIE_y_b + CIE_x_b*CIE_y_r     ) )
#define CIE_C_bD        ( (1./CIE_y_w) *                                 ( CIE_x_w*(CIE_y_r - CIE_y_g) -                                   CIE_y_w*(CIE_x_r - CIE_x_g) +                                   CIE_x_r*CIE_y_g - CIE_x_g*CIE_y_r     ) )

#define CIE_rf          (CIE_y_r*CIE_C_rD/CIE_D)
#define CIE_gf          (CIE_y_g*CIE_C_gD/CIE_D)
#define CIE_bf          (CIE_y_b*CIE_C_bD/CIE_D)

/* As of 9-94, CIE_rf=.265074126, CIE_gf=.670114631 and CIE_bf=.064811243 */

/***** The following definitions are valid for RGB colors only... *****/

#define  bright(col)    (CIE_rf*(col)[RED_]+CIE_gf*(col)[GRN]+CIE_bf*(col)[BLU])

/* luminous efficacies over visible spectrum */
#define  WHTEFFICACY            179.            /* uniform white light */

#define  luminance(col)         (WHTEFFICACY * bright(col))

/***** ...end of stuff specific to RGB colors *****/

/*
 * Conversions to and from XYZ space generally don't apply WHTEFFICACY.
 * If you need Y to be luminance (cd/m^2), this must be applied when
 * converting from radiance (watts/sr/m^2).
 */

extern COLORMAT globalRgb2XyzMat;            /* RGB to XYZ conversion matrix */
extern COLORMAT globalXyz2RgbMat;            /* XYZ to RGB conversion matrix */

char *tempbuffer(unsigned int len);                /* get a temporary buffer */
int fwritecolrs(COLR *scanline, int len, FILE *fp); /* write out a colr scanline */
int freadcolrs(COLR *scanline, int len, FILE *fp); /* read in an encoded colr scanline */
int oldreadcolrs(COLR *scanline, int len, FILE *fp); /* read in an old colr scanline */
int fwritescan(COLOR *scanline, int len, FILE *fp); /* write out a scanline */
int setcolr(COLR clr, double r, double g, double b); /* assign a short color value */
int colr_color(COLOR col, COLR clr); /* convert short to float color */
