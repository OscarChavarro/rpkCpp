#ifndef IMAGE_PPM_H
#define IMAGE_PPM_H

#include "PixelRGB.h"

class ImagePPM {
public:
    int width;
    int height;
    PixelRGB* data;

    ImagePPM();
    ~ImagePPM();

    int allocate(int w, int h);
    PixelRGB get(int x, int y) const;
    void set(int x, int y, PixelRGB p);
};

#endif
