#include <cmath>

#include "common/CppReAlloc.h"
#include "io/image/dkcolor.h"

static const int RED = 0;
static const int GREEN = 1;
static const int BLUE = 2;

// Exponent same for either format
static const int EXP = 3;

// Excess used for exponent
static const int COL_XS = 128;

// Minimum scanline length for encoding
static const int MINIMUM_SCAN_LINE_LENGTH = 8;

// Maximum scanline length for encoding
static const int MAXIMUM_SCAN_LINE_LENGTH = 0x7fff;
static const int MINIMUM_RUN_LENGTH = 4;
static BYTE *globalTempBuffer = nullptr;

/**
Get a temporary buffer
*/
static BYTE *
dkColorTempBuffer(unsigned int length) {
    static unsigned tempBufferLength = 0;

    if ( length > tempBufferLength ) {
        if ( tempBufferLength > 0 ) {
            CppReAlloc<BYTE> memoryManager;
            globalTempBuffer = memoryManager.reAlloc(globalTempBuffer, length);
        } else {
            globalTempBuffer = new BYTE[length];
        }
        tempBufferLength = globalTempBuffer == nullptr ? 0 : length;
    }
    return globalTempBuffer;
}

/**
Write out a byte color scanline
*/
static int
dkColorWriteByteColors(BYTE_COLOR *scanline, int len, FILE *fp) {
    int cnt = 0;
    int c2;

    if ( len < MINIMUM_SCAN_LINE_LENGTH || len > MAXIMUM_SCAN_LINE_LENGTH ) {
        // OOBs, write out flat
        return (int)(fwrite((char *) scanline, sizeof(BYTE_COLOR), len, fp) - len);
    }

    // Put magic header
    putc(2, fp);
    putc(2, fp);
    putc(len >> 8, fp);
    putc(len & 255, fp);

    // Put components separately
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < len; j += cnt ) {
            // Find next run
            int beg;

            for ( beg = j; beg < len; beg += cnt ) {
                for ( cnt = 1; cnt < 127 && beg + cnt < len && scanline[beg + cnt][i] == scanline[beg][i]; cnt++ );
                if ( cnt >= MINIMUM_RUN_LENGTH ) {
                    // Long enough
                    break;
                }
            }

            if ( beg - j > 1 && beg - j < MINIMUM_RUN_LENGTH ) {
                c2 = j + 1;
                while ( scanline[c2++][i] == scanline[j][i] ) {
                    if ( c2 == beg ) {
                        // Short run
                        putc(128 + beg - j, fp);
                        putc(scanline[j][i], fp);
                        j = beg;
                        break;
                    }
                }
            }
            while ( j < beg ) {
                // Write out non-run
                if ( (c2 = beg - j) > 128 ) {
                    c2 = 128;
                }
                putc(c2, fp);
                while ( c2-- ) {
                    putc(scanline[j++][i], fp);
                }
            }
            if ( cnt >= MINIMUM_RUN_LENGTH ) {
                // Write out run
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
Assign a short color value
*/
static void
dkColorSetByteColors(BYTE_COLOR clr, double r, double g, double b)
{
    double d;
    int e;

    d = r > g ? r : g;
    if ( b > d ) {
        d = b;
    }

    if ( d <= 1e-32 ) {
        clr[RED] = clr[GREEN] = clr[BLUE] = 0;
        clr[EXP] = 0;
        return;
    }

    d = std::frexp(d, &e) * 255.9999 / d;

    clr[RED] = (unsigned char) (r * d);
    clr[GREEN] = (unsigned char) (g * d);
    clr[BLUE] = (unsigned char) (b * d);
    clr[EXP] = (unsigned char)e + COL_XS;
}

/**
Write out a scanline
*/
int
dkColorWriteScan(COLOR *scanline, int len, FILE *fp)
{
    // Get scanline buffer
    BYTE *byteArray = dkColorTempBuffer(len * sizeof(BYTE_COLOR));
    BYTE_COLOR *sp = (BYTE_COLOR *)byteArray;
    if ( sp == nullptr ) {
        return -1;
    }
    BYTE_COLOR *colorScan = sp;

    // Convert scanline
    int n = len;
    while ( n-- > 0 ) {
        dkColorSetByteColors(sp[0], scanline[0][RED], scanline[0][GREEN], scanline[0][BLUE]);
        scanline++;
        sp++;
    }
    return dkColorWriteByteColors(colorScan, len, fp);
}

void
dkColorFreeBuffer() {
    if ( globalTempBuffer != nullptr ) {
        delete[] globalTempBuffer;
        globalTempBuffer = nullptr;
    }
}
