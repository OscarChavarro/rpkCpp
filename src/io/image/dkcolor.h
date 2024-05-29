#ifndef __DK_COLOR__
#define __DK_COLOR__

/**
Routines using pixel color values / color calculations.
     12/31/85

Two color representations are used, one for calculation and
another for storage.  Calculation is done with three floats
for speed.  Stored color values use 4 bytes which contain
three single byte mantissas and a common exponent.
*/

#include <cstdio>

typedef unsigned char BYTE; // 8-bit unsigned integer
typedef BYTE BYTE_COLOR[4]; // Red, green, blue (or X,Y,Z), exponent
typedef float DK_COLOR[3]; // Red, green, blue (or X,Y,Z)

int dkColorWriteScan(DK_COLOR *scanline, int len, FILE *fileDescriptor);
void dkColorFreeBuffer();

#endif
