#include "common/CppReAlloc.h"
#include "io/image/dkcolor.h"
#include "io/wrapper/PersistenceElement.h"
#include "java/lang/Math.h"

static constexpr int RED = 0;
static constexpr int GREEN = 1;
static constexpr int BLUE = 2;

// Exponent same for either format
static constexpr int EXP = 3;

// Excess used for exponent
static constexpr int COL_XS = 128;

// Minimum scanline length for encoding
static constexpr int MINIMUM_SCAN_LINE_LENGTH = 8;

// Maximum scanline length for encoding
static constexpr int MAXIMUM_SCAN_LINE_LENGTH = 0x7fff;
static constexpr int MINIMUM_RUN_LENGTH = 4;
static BYTE *globalTempBuffer = nullptr;

static inline void
dkColorWriteByte(java::io::OutputStream *stream, int value) {
    if ( stream == nullptr ) {
        return;
    }
    stream->write(value & 0xFF);
}

/**
Get a temporary buffer
*/
static BYTE *
dkColorTempBuffer(unsigned int length) {
    static unsigned tempBufferLength = 0;

    if ( length > tempBufferLength ) {
        if ( tempBufferLength > 0 ) {
            globalTempBuffer = CppReAlloc::reAlloc(
                globalTempBuffer,
                static_cast<int>(tempBufferLength),
                static_cast<int>(length));
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
dkColorWriteByteColors(BYTE_COLOR *scanline, int len, java::io::OutputStream *outputStream) {
    int cnt = 0;
    int c2;

    if ( outputStream == nullptr ) {
        return -1;
    }

    if ( len < MINIMUM_SCAN_LINE_LENGTH || len > MAXIMUM_SCAN_LINE_LENGTH ) {
        // OOBs, write out flat
        vsdk::PersistenceElement::writeBytes(
            *outputStream,
            reinterpret_cast<unsigned char *>(scanline),
            static_cast<int>(sizeof(BYTE_COLOR) * len));
        return 0;
    }

    // Put magic header
    dkColorWriteByte(outputStream, 2);
    dkColorWriteByte(outputStream, 2);
    dkColorWriteByte(outputStream, len >> 8);
    dkColorWriteByte(outputStream, len & 255);

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
                        dkColorWriteByte(outputStream, 128 + beg - j);
                        dkColorWriteByte(outputStream, scanline[j][i]);
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
                dkColorWriteByte(outputStream, c2);
                while ( c2-- ) {
                    dkColorWriteByte(outputStream, scanline[j++][i]);
                }
            }
            if ( cnt >= MINIMUM_RUN_LENGTH ) {
                // Write out run
                dkColorWriteByte(outputStream, 128 + cnt);
                dkColorWriteByte(outputStream, scanline[beg][i]);
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
static void
dkColorSetByteColors(BYTE_COLOR color, double r, double g, double b)
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

    const int e = java::Math::getExponent(d) + 1;
    const double normalized = java::Math::scalb(d, -e);
    d = normalized * 255.9999 / d;

    color[RED] = static_cast<unsigned char>(r * d);
    color[GREEN] = static_cast<unsigned char>(g * d);
    color[BLUE] = static_cast<unsigned char>(b * d);
    color[EXP] = static_cast<unsigned char>(e + COL_XS);
}

/**
Write out a scanline
*/
int
dkColorWriteScan(DK_COLOR *scanline, int len, java::io::OutputStream *outputStream)
{
    // Get scanline buffer
    BYTE *byteArray = dkColorTempBuffer(len * sizeof(BYTE_COLOR));
    BYTE_COLOR *sp = reinterpret_cast<BYTE_COLOR *>(byteArray);
    if ( sp == nullptr ) {
        return -1;
    }
    BYTE_COLOR *colorScan = sp;

    // Convert scanline
    for ( int n = 0; n < len; n++ ) {
        dkColorSetByteColors(sp[n], scanline[n][RED], scanline[n][GREEN], scanline[n][BLUE]);
    }
    return dkColorWriteByteColors(colorScan, len, outputStream);
}

void
dkColorFreeBuffer() {
    if ( globalTempBuffer != nullptr ) {
        delete[] globalTempBuffer;
        globalTempBuffer = nullptr;
    }
}
