package vsdk.toolkit.io.image;

/**
Routines using pixel color values / color calculations.
     12/31/85

Two color representations are used, one for calculation and
another for storage.  Calculation is done with three floats
for speed.  Stored color values use 4 bytes which contain
three single byte mantissas and a common exponent.
*/

import java.io.OutputStream;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.CppReAlloc;
import vsdk.toolkit.io.PersistenceElement;

public class DkColor {
    private static final int RED = 0;
    private static final int GREEN = 1;
    private static final int BLUE = 2;
    private static final int EXP = 3;
    private static final int COL_XS = 128;
    private static final int MINIMUM_SCAN_LINE_LENGTH = 8;
    private static final int MAXIMUM_SCAN_LINE_LENGTH = 0x7fff;
    private static final int MINIMUM_RUN_LENGTH = 4;

    private static byte[] temporaryBuffer = null;
    private static int temporaryBufferLength = 0;

    private static void writeByte(OutputStream stream, int value) {
        if (stream == null) {
            return;
        }
        try {
            stream.write(value & 0xFF);
        }
        catch (Exception ignored) {
        }
    }

    /**
Get a temporary buffer
*/
    private static byte[] tempBuffer(int length) {
        if (length > temporaryBufferLength) {
            if (temporaryBufferLength > 0) {
                temporaryBuffer = CppReAlloc.reAlloc(
                    temporaryBuffer,
                    temporaryBufferLength,
                    length);
            }
            else {
                temporaryBuffer = new byte[length];
            }
            temporaryBufferLength = temporaryBuffer == null ? 0 : length;
        }
        return temporaryBuffer;
    }

    /**
Write out a byte color scanline
*/
    private static int writeByteColors(byte[] scanline, int len, OutputStream outputStream) {
        int cnt = 0;
        int c2;

        if (outputStream == null) {
            return -1;
        }

        if (len < MINIMUM_SCAN_LINE_LENGTH || len > MAXIMUM_SCAN_LINE_LENGTH) {
            // OOBs, write out flat
            PersistenceElement.writeBytes(outputStream, scanline, len * 4);
            return 0;
        }

        // Put magic header
        writeByte(outputStream, 2);
        writeByte(outputStream, 2);
        writeByte(outputStream, len >> 8);
        writeByte(outputStream, len & 255);

        // Put components separately
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < len; j += cnt) {
                // Find next run
                int beg;

                for (beg = j; beg < len; beg += cnt) {
                    for (cnt = 1;
                         cnt < 127
                         && beg + cnt < len
                         && scanline[(beg + cnt) * 4 + i] == scanline[beg * 4 + i];
                         cnt++) {
                    }
                    if (cnt >= MINIMUM_RUN_LENGTH) {
                        // Long enough
                        break;
                    }
                }

                if (beg - j > 1 && beg - j < MINIMUM_RUN_LENGTH) {
                    c2 = j + 1;
                    while (c2 < beg && scanline[c2 * 4 + i] == scanline[j * 4 + i]) {
                        c2++;
                        if (c2 == beg) {
                            // Short run
                            writeByte(outputStream, 128 + beg - j);
                            writeByte(outputStream, scanline[j * 4 + i]);
                            j = beg;
                            break;
                        }
                    }
                }
                while (j < beg) {
                    // Write out non-run
                    if ( (c2 = beg - j) > 128 ) {
                        c2 = 128;
                    }
                    writeByte(outputStream, c2);
                    while (c2-- > 0) {
                        writeByte(outputStream, scanline[j * 4 + i]);
                        j++;
                    }
                }
                if (cnt >= MINIMUM_RUN_LENGTH) {
                    // Write out run
                    writeByte(outputStream, 128 + cnt);
                    writeByte(outputStream, scanline[beg * 4 + i]);
                }
                else {
                    cnt = 0;
                }
            }
        }
        return 0;
    }

    /**
Assign a short color value
*/
    private static void setByteColors(byte[] scanline, int base, double r, double g, double b)
    {
        double d = r > g ? r : g;
        if (b > d) {
            d = b;
        }

        if (d < 0) {
            scanline[base + RED] = 0;
            scanline[base + GREEN] = 0;
            scanline[base + BLUE] = 0;
            scanline[base + EXP] = 0;
            return;
        }

        final int e = Math.getExponent(d) + 1;
        final double normalized = Math.scalb(d, -e);
        d = normalized * 255.9999 / d;

        scanline[base + RED] = (byte)(int)(r * d);
        scanline[base + GREEN] = (byte)(int)(g * d);
        scanline[base + BLUE] = (byte)(int)(b * d);
        scanline[base + EXP] = (byte)(int)(e + COL_XS);
    }

    /**
Write out a scanline
*/
    public static int writeScan(float[][] scanline, int len, OutputStream outputStream)
    {
        // Get scanline buffer
        byte[] colorScan = tempBuffer(len * 4);
        if (colorScan == null) {
            return -1;
        }

        // Convert scanline
        for (int n = 0; n < len; n++) {
            int base = n * 4;
            setByteColors(colorScan, base, scanline[n][RED], scanline[n][GREEN], scanline[n][BLUE]);
        }
        return writeByteColors(colorScan, len, outputStream);
    }

    public static int writeScan(ColorRgb[] scanline, int len, OutputStream outputStream)
    {
        // Get scanline buffer
        byte[] colorScan = tempBuffer(len * 4);
        if (colorScan == null) {
            return -1;
        }

        // Convert scanline
        for (int n = 0; n < len; n++) {
            int base = n * 4;
            setByteColors(colorScan, base, scanline[n].r, scanline[n].g, scanline[n].b);
        }
        return writeByteColors(colorScan, len, outputStream);
    }

    public static void freeBuffer() {
        if (temporaryBuffer != null) {
            temporaryBuffer = null;
            temporaryBufferLength = 0;
        }
    }
}
