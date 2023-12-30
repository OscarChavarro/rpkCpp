#ifndef _PPM_H_
#define _PPM_H_

#include "IMAGE/imagecpp.h"

class PPMOutputHandle : public ImageOutputHandle {
  private:
    FILE *fp;

  public:
    PPMOutputHandle(FILE *_fp, int _width, int _height);
    int WriteDisplayRGB(unsigned char *rgb);
};

#endif
