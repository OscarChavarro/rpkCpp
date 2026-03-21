#include <stdlib.h>

#include "ImagePPM.h"

ImagePPM::ImagePPM() {
    width = 0;
    height = 0;
    data = NULL;
}

ImagePPM::~ImagePPM() {
    if (data != NULL) {
        free(data);
    }
}

int ImagePPM::allocate(int w, int h) {
    width = w;
    height = h;
    data = (PixelRGB*)malloc(sizeof(PixelRGB) * w * h);
    if (data == NULL) {
        return 0;
    }
    return 1;
}

PixelRGB ImagePPM::get(int x, int y) const {
    return data[y * width + x];
}

void ImagePPM::set(int x, int y, PixelRGB p) {
    data[y * width + x] = p;
}
