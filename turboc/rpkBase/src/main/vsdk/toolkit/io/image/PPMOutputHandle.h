#ifndef __PPM_OUTPUT_HANDLE__
#define __PPM_OUTPUT_HANDLE__

#include "java/io/OutputStream.h"
#include "vsdk/toolkit/io/image/ImageOutputHandle.h"

class PPMOutputHandle: public ImageOutputHandle{ private:
    OutputStream *outputStream;

  public:
    PPMOutputHandle(OutputStream *_outputStream, int _width, int _height);
    int writeDisplayRGB(unsigned char *rgb);
};

#endif
