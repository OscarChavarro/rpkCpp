#ifndef __PPM__
#define __PPM__

#include "java/io/OutputStream.h"

#include "io/image/ImageOutputHandle.h"

class PPMOutputHandle final : public ImageOutputHandle {
  private:
    java::io::OutputStream *outputStream;

  public:
    PPMOutputHandle(java::io::OutputStream *_outputStream, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb) final;
};

#endif
