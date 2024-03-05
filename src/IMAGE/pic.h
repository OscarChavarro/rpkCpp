#ifndef __PIC_CPP__
#define __PIC_CPP__

#include "IMAGE/imagecpp.h"

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
class PicOutputHandle : public ImageOutputHandle {
  protected:
    FILE *pic;

    void writeHeader();

  public:
    PicOutputHandle(char *filename, int w, int h);
    ~PicOutputHandle();
    int writeRadianceRGB(float *rgbRadiance);
};

#endif
