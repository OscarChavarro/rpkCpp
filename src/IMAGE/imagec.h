#ifndef __IMAGE__
#define __IMAGE__

#include "IMAGE/imagecpp.h"

/**
Declares functions to create an ImageOutputHandle that
examines the filename in order to decide what format to use. The following
ImageOutputHandle constructors are only needed if you want to specify
yourself what format to use
*/

extern char *imageFileExtension(char *fileName);
extern int writeDisplayRGB(ImageOutputHandle *img, unsigned char *data);
extern void deleteImageOutputHandle(ImageOutputHandle *img);

#endif
