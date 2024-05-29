#ifndef __PIC_CPP__
#define __PIC_CPP__

#include "io/image/ImageOutputHandle.h"

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
class PicOutputHandle final : public ImageOutputHandle {
  private:
    FILE *fileDescriptor;

    void writeHeader();

  public:
    PicOutputHandle(const char *filename, int w, int h);
    ~PicOutputHandle() final;
    int writeRadianceRGB(ColorRgb *rgbRadiance) final;
};

#endif
