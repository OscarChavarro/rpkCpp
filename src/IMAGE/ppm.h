#ifndef __PPM__
#define __PPM__

#include "IMAGE/imagecpp.h"

class PPMOutputHandle : public ImageOutputHandle {
  private:
    FILE *fp;

  public:
    PPMOutputHandle(FILE *_fp, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb);
};

#endif
