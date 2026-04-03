#ifndef PPM_READER_H
#define PPM_READER_H

#include "ImagePPM.h"

class PPMReader {
public:
    static int read(const char* filename, ImagePPM& img);
};

#endif
