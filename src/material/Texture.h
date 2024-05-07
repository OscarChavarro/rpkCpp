#ifndef __TEXTURE__
#define __TEXTURE__

#include "common/ColorRgb.h"

class Texture {
  private:
    int width;
    int height;
    int channels;
    unsigned char *data; // First bytes correspond to bottom-left pixel (as in OpenGL)

  public:
    ColorRgb evaluateColor(float u, float v) const;
};

#endif
