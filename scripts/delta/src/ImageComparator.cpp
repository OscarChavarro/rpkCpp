#include "ImageComparator.h"
#include "Stats.h"

double ImageComparator::absd(double v) {
    if (v < 0) {
        return -v;
    }
    return v;
}

double ImageComparator::sqrt_approx(double x) {
    if (x <= 0) {
        return 0;
    }

    double guess = x;
    for (int i = 0; i < 10; i++) {
        guess = 0.5 * (guess + x / guess);
    }
    return guess;
}

double ImageComparator::distance(PixelRGB a, PixelRGB b) {
    double dr = (double)a.r - (double)b.r;
    double dg = (double)a.g - (double)b.g;
    double db = (double)a.b - (double)b.b;
    return sqrt_approx(dr * dr + dg * dg + db * db);
}

int ImageComparator::compare(ImagePPM& a, ImagePPM& b, ImagePPM& out, double threshold) {
    if (a.width != b.width || a.height != b.height) {
        return 0;
    }

    if (!out.allocate(a.width, a.height)) {
        return 0;
    }

    Stats stats;

    for (int y = 0; y < a.height; y++) {
        for (int x = 0; x < a.width; x++) {
            PixelRGB pa = a.get(x, y);
            PixelRGB pb = b.get(x, y);
            stats.trackPixel(pa);
            stats.trackPixel(pb);
            double d = distance(pa, pb);
            stats.trackDifference(d);

            PixelRGB result;

            if (d > threshold) {
                result.r = 255;
                result.g = 0;
                result.b = 0;
            } else {
                result.r = 0;
                result.g = 0;
                result.b = 0;
            }

            out.set(x, y, result);
        }
    }

    stats.printReport();

    return 1;
}
