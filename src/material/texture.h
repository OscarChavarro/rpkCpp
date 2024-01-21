#ifndef __TEXTURE__
#define __TEXTURE__

#include "material/color.h"

class TEXTURE {
  public:
    int width;
    int height;
    int channels;
    unsigned char *data; // First bytes correspond to bottom-left pixel (as in OpenGL)
};

extern void printTexture(FILE *out, TEXTURE *t);
extern COLOR evalTextureColor(TEXTURE *texture, float u, float v);

#endif
