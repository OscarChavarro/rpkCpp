#ifndef __PPM_WRITER__
#define __PPM_WRITER__

#include "ImagePPM.h"

class PPMWriter {
public:
    static int write(const char* filename, ImagePPM& img);
};

#endif
