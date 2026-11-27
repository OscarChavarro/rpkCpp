package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.color.ColorRgb;

/**
Operations over contiguous arrays of colors (Galerkin basis coefficients).
Single-color arithmetic lives in ColorRgb.
*/
public final class ColorRgbMath {
    private ColorRgbMath() {
    }

    public static void arrayCopy(ColorRgb[] dest, ColorRgb[] src, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = src[i];
        }
    }

    public static void arrayAdd(ColorRgb[] dest, ColorRgb[] src, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = dest[i].add(src[i]);
        }
    }

    public static void arrayClear(ColorRgb[] dest, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = new ColorRgb(0.0, 0.0, 0.0);
        }
    }
}
