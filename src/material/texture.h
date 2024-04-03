#ifndef __TEXTURE__
#define __TEXTURE__

#include "common/ColorRgb.h"

class TEXTURE {
  public:
    int width;
    int height;
    int channels;
    unsigned char *data; // First bytes correspond to bottom-left pixel (as in OpenGL)
};

extern ColorRgb evalTextureColor(TEXTURE *texture, float u, float v);

#endif
