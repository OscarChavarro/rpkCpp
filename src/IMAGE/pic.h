#ifndef __PIC_H__
#define __PIC_H__

#include "IMAGE/imagecpp.h"

/**
 *	(Simple) High dynamic range PIC output handle.
 *
 *	Olaf Appeltants, March 2000
 */
class PicOutputHandle : public ImageOutputHandle {
protected:
    FILE *pic;

    void writeHeader();

public:
    PicOutputHandle(char *filename, int w, int h);

    ~PicOutputHandle();

    int writeRadianceRGB(float *rgbrad);
};

#endif
