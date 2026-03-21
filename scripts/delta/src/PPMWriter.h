#ifndef PPM_WRITER_H
#define PPM_WRITER_H

#include "ImagePPM.h"

class PPMWriter {
public:
    static int write(const char* filename, ImagePPM& img);
};

#endif
