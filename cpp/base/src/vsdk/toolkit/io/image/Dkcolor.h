#ifndef DK_COLOR__
#define DK_COLOR__

/**
Routines using pixel color values / color calculations.
     12/31/85

Two color representations are used, one for calculation and
another for storage.  Calculation is done with three floats
for speed.  Stored color values use 4 bytes which contain
three single byte mantissas and a common exponent.
*/

#include "vsdk/toolkit/java/io/OutputStream.h"

using BYTE = unsigned char; // 8-bit unsigned integer
using BYTE_COLOR = BYTE[4]; // Red, green, blue (or X,Y,Z), exponent
using DK_COLOR = float[3]; // Red, green, blue (or X,Y,Z)

class DkColor {
  public:
    static int writeScan(DK_COLOR *scanline, int len, java::OutputStream *outputStream);
    static void freeBuffer();

  private:
    static constexpr int RED = 0;
    static constexpr int GREEN = 1;
    static constexpr int BLUE = 2;
    static constexpr int EXP = 3;
    static constexpr int COL_XS = 128;
    static constexpr int MINIMUM_SCAN_LINE_LENGTH = 8;
    static constexpr int MAXIMUM_SCAN_LINE_LENGTH = 0x7fff;
    static constexpr int MINIMUM_RUN_LENGTH = 4;

    static BYTE *temporaryBuffer;
    static unsigned int temporaryBufferLength;

    static void writeByte(java::OutputStream *stream, int value);
    static BYTE *tempBuffer(unsigned int length);
    static int writeByteColors(BYTE_COLOR *scanline, int len, java::OutputStream *outputStream);
    static void setByteColors(BYTE_COLOR color, double r, double g, double b);
};

#endif
