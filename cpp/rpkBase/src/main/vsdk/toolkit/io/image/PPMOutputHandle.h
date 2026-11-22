#ifndef PPM__
#define PPM__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/io/image/ImageOutputHandle.h"

class PPMOutputHandle final : public ImageOutputHandle {
  private:
    java::OutputStream *outputStream;

  public:
    PPMOutputHandle(java::OutputStream *_outputStream, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb) final;
};

#endif
