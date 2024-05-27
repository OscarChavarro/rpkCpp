#ifndef __PPM__
#define __PPM__

#include "io/image/ImageOutputHandle.h"

class PPMOutputHandle final : public ImageOutputHandle {
  private:
    FILE *fp;

  public:
    PPMOutputHandle(FILE *_fp, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb) final;
};

#endif
