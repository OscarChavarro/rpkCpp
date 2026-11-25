#ifndef __PPM_READER__
#define __PPM_READER__

#include "ImagePPM.h"

class PPMReader {
public:
    static int read(const char* filename, ImagePPM& img);
};

#endif
