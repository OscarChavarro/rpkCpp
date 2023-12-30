#ifndef _RPK_TEXTURE_H_
#define _RPK_TEXTURE_H_

#include <cstdio>
#include "material/color.h"

class TEXTURE {
  public:
    int width, height, channels;
    unsigned char *data;  // first bytes correspond to bottom-left pixel (as in OpenGL)
};

extern void PrintTexture(FILE *out, TEXTURE *texture);
extern COLOR EvalTextureColor(TEXTURE *texture, float u, float v);

#endif
