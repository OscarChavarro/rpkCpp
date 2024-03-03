/**
Routines for color calculations. 10/10/85
*/

#include <cstdlib>
#include "common/mymath.h"
#include "IMAGE/dkcolor.h"

#define MINIMUM_SCAN_LINE_LENGTH 8 /* minimum scanline length for encoding */
#define MAXIMUM_SCAN_LINE_LENGTH 0x7fff /* maximum scanline length for encoding */
#define MINIMUM_RUN_LENGTH 4

/**
Get a temporary buffer
*/
char *
tempbuffer(unsigned int len)
{
    static char *tempbuf = nullptr;
    static unsigned tempbuflen = 0;

    if ( len > tempbuflen ) {
        if ( tempbuflen > 0 ) {
            tempbuf = (char *)realloc(tempbuf, len);
        } else {
            tempbuf = (char *)malloc(len);
        }
        tempbuflen = tempbuf == nullptr ? 0 : len;
    }
    return (tempbuf);
}

int
fwritecolrs(COLR *scanline, int len, FILE *fp)/* write out a colr scanline */
{
    int i, j, beg, cnt = 0;
    int c2;

    if ( len < MINIMUM_SCAN_LINE_LENGTH || len > MAXIMUM_SCAN_LINE_LENGTH ) {      /* OOBs, write out flat */
        return (fwrite((char *) scanline, sizeof(COLR), len, fp) - len);
    }
    /* put magic header */
    putc(2, fp);
    putc(2, fp);
    putc(len >> 8, fp);
    putc(len & 255, fp);
    /* put components seperately */
    for ( i = 0; i < 4; i++ ) {
        for ( j = 0; j < len; j += cnt ) {    /* find next run */
            for ( beg = j; beg < len; beg += cnt ) {
                for ( cnt = 1; cnt < 127 && beg + cnt < len &&
                               scanline[beg + cnt][i] == scanline[beg][i]; cnt++ ) {
                }
                if ( cnt >= MINIMUM_RUN_LENGTH ) {
                    break;
                }                  /* long enough */
            }
            if ( beg - j > 1 && beg - j < MINIMUM_RUN_LENGTH ) {
                c2 = j + 1;
                while ( scanline[c2++][i] == scanline[j][i] ) {
                    if ( c2 == beg ) {        /* short run */
                        putc(128 + beg - j, fp);
                        putc(scanline[j][i], fp);
                        j = beg;
                        break;
                    }
                }
            }
            while ( j < beg ) {               /* write out non-run */
                if ((c2 = beg - j) > 128 ) {
                    c2 = 128;
                }
                putc(c2, fp);
                while ( c2-- ) {
                    putc(scanline[j++][i], fp);
                }
            }
            if ( cnt >= MINIMUM_RUN_LENGTH ) {            /* write out run */
                putc(128 + cnt, fp);
                putc(scanline[beg][i], fp);
            } else {
                cnt = 0;
            }
        }
    }
    return (ferror(fp) ? -1 : 0);
}

/**
Write out a scanline
*/
int
fwritescan(COLOR *scanline, int len, FILE *fp)
{
    COLR *clrscan;
    int n;
    COLR *sp;
    // get scanline buffer
    if ((sp = (COLR *) tempbuffer(len * sizeof(COLR))) == nullptr) {
        return (-1);
    }
    clrscan = sp;
    // Convert scanline
    n = len;
    while ( n-- > 0 ) {
        setcolr(sp[0], scanline[0][RED_],
                scanline[0][GRN],
                scanline[0][BLU]);
        scanline++;
        sp++;
    }
    return (fwritecolrs(clrscan, len, fp));
}

int
setcolr(COLR clr, double r, double g, double b)   /* assign a short color value */
{
    double d;
    int e;

    d = r > g ? r : g;
    if ( b > d ) {
        d = b;
    }

    if ( d <= 1e-32 ) {
        clr[RED_] = clr[GRN] = clr[BLU] = 0;
        clr[EXP] = 0;
        return (0);
    }

    d = frexp(d, &e) * 255.9999 / d;

    clr[RED_] = (unsigned char) (r * d);
    clr[GRN] = (unsigned char) (g * d);
    clr[BLU] = (unsigned char) (b * d);
    clr[EXP] = e + COLXS;
    return (0);
}
