#ifndef __PPM__
#define __PPM__

#include "io/image/ImageOutputHandle.h"
#include "java/io/OutputStream.h"

class PPMOutputHandle final : public ImageOutputHandle {
  private:
    FILE *fp;
    java::io::OutputStream *outputStream;

  public:
    PPMOutputHandle(FILE *_fp, int _width, int _height);
    PPMOutputHandle(java::io::OutputStream *_outputStream, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb) final;
};

#endif
