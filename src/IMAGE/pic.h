#ifndef __PIC_CPP__
#define __PIC_CPP__

#include "IMAGE/ImageOutputHandle.h"

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
class PicOutputHandle final : public ImageOutputHandle {
  private:
    FILE *pic;

    void writeHeader();

  public:
    PicOutputHandle(const char *filename, int w, int h);
    ~PicOutputHandle() final;
    int writeRadianceRGB(ColorRgb *rgbRadiance) final;
};

#endif
