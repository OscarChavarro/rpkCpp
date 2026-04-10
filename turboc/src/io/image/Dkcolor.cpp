#include "java/lang/Math.h"
#include "common/CppReAlloc.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/image/Dkcolor.h"

BYTE *DkColor::temporaryBuffer = NULL;
unsigned int DkColor::temporaryBufferLength = 0;

void
DkColor::writeByte(OutputStream *stream, int value) {
    if ( stream == NULL ) {
        return;
    }
    stream->write(value & 0xFF);
}

/**
Get a temporary buffer
*/
BYTE *
DkColor::tempBuffer(unsigned int length) {
    if ( length > temporaryBufferLength ) {
        if ( temporaryBufferLength > 0 ) {
            temporaryBuffer = CppReAlloc::reAlloc(
                temporaryBuffer,
                ((int)(temporaryBufferLength)),
                ((int)(length)));
        } else {
            temporaryBuffer = new BYTE[length];
        }
        temporaryBufferLength = temporaryBuffer == NULL ? 0 : length;
    }
    return temporaryBuffer;
}

/**
Write out a byte color scanline
*/
int
DkColor::writeByteColors(BYTE_COLOR *scanline, int len, OutputStream *outputStream) {
    int cnt = 0;
    int c2;

    if ( outputStream == NULL ) {
        return -1;
    }

    if ( len < MINIMUM_SCAN_LINE_LENGTH || len > MAXIMUM_SCAN_LINE_LENGTH ) {
        // OOBs, write out flat
        PersistenceElement::writeBytes(
            *outputStream,
            ((unsigned char *)(scanline)),
            ((int)(sizeof(BYTE_COLOR) * len)));
        return 0;
    }

    // Put magic header
    writeByte(outputStream, 2);
    writeByte(outputStream, 2);
    writeByte(outputStream, len >> 8);
    writeByte(outputStream, len & 255);

    // Put components separately
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < len; j += cnt ) {
            // Find next run
            int beg;

            for ( beg = j; beg < len; beg += cnt ) {
                for ( cnt = 1; cnt < 127 && beg + cnt < len && scanline[beg + cnt][i] == scanline[beg][i]; cnt++ ) {}
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
                        writeByte(outputStream, 128 + beg - j);
                        writeByte(outputStream, scanline[j][i]);
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
                writeByte(outputStream, c2);
                while ( c2-- ) {
                    writeByte(outputStream, scanline[j++][i]);
                }
            }
            if ( cnt >= MINIMUM_RUN_LENGTH ) {
                // Write out run
                writeByte(outputStream, 128 + cnt);
                writeByte(outputStream, scanline[beg][i]);
            } else {
                cnt = 0;
            }
        }
    }
    return 0;
}

/**
Assign a short color value
*/
void
DkColor::setByteColors(BYTE_COLOR color, double r, double g, double b)
{
    double d = r > g ? r : g;
    if ( b > d ) {
        d = b;
    }

    if ( d < 0 ) {
        color[RED] = 0;
        color[GREEN] = 0;
        color[BLUE] = 0;
        color[EXP] = 0;
        return;
    }

    const int e = Math::getExponent(d) + 1;
    const double normalized = Math::scalb(d, -e);
    d = normalized * 255.9999 / d;

    color[RED] = ((unsigned char)(r * d));
    color[GREEN] = ((unsigned char)(g * d));
    color[BLUE] = ((unsigned char)(b * d));
    color[EXP] = ((unsigned char)(e + COL_XS));
}

/**
Write out a scanline
*/
int
DkColor::writeScan(DK_COLOR *scanline, int len, OutputStream *outputStream)
{
    // Get scanline buffer
    BYTE *byteArray = tempBuffer(len * sizeof(BYTE_COLOR));
    BYTE_COLOR *sp = ((BYTE_COLOR *)(byteArray));
    if ( sp == NULL ) {
        return -1;
    }
    BYTE_COLOR *colorScan = sp;

    // Convert scanline
    for ( int n = 0; n < len; n++ ) {
        setByteColors(sp[n], scanline[n][RED], scanline[n][GREEN], scanline[n][BLUE]);
    }
    return writeByteColors(colorScan, len, outputStream);
}

void
DkColor::freeBuffer() {
    if ( temporaryBuffer != NULL ) {
        delete[] temporaryBuffer;
        temporaryBuffer = NULL;
        temporaryBufferLength = 0;
    }
}
