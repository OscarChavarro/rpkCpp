#ifndef __TEXTURE__
#define __TEXTURE__

#include "common/ColorRgb.h"

class Texture {
  public:
    int width;
    int height;
    int channels;
    unsigned char *data; // First bytes correspond to bottom-left pixel (as in OpenGL)
    ColorRgb evaluateColor(float u, float v) const;
};

#endif
