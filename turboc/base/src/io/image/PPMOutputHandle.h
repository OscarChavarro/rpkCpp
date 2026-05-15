#ifndef __PPM__
#define __PPM__

#include "java/io/OutputStream.h"
#include "io/image/ImageOutputHandle.h"

class PPMOutputHandle: public ImageOutputHandle{ private:
    OutputStream *outputStream;

  public:
    PPMOutputHandle(OutputStream *_outputStream, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb);
};

#endif
