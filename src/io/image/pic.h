#ifndef __PIC_CPP__
#define __PIC_CPP__

#include "java/io/OutputStream.h"

#include "io/image/ImageOutputHandle.h"

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
class PicOutputHandle final : public ImageOutputHandle {
  private:
    java::io::OutputStream *outputStream;

    void writeHeader();

  public:
    PicOutputHandle(const char *filename, int w, int h);
    ~PicOutputHandle() final;
    int writeRadianceRGB(ColorRgb *rgbRadiance) final;
};

#endif
