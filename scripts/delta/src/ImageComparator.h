#ifndef IMAGE_COMPARATOR_H
#define IMAGE_COMPARATOR_H

#include "ImagePPM.h"

class ImageComparator {
public:
    static double absd(double v);
    static double sqrt_approx(double x);
    static double distance(PixelRGB a, PixelRGB b);
    static int compare(ImagePPM& a, ImagePPM& b, ImagePPM& out, double threshold);
};

#endif
