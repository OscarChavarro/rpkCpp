/* image.h: ANSI-C interface to the IMAGE library */

#ifndef _IMAGE_h_
#define _IMAGE_h_

#include "IMAGE/imagecpp.h"

/* image.H declares already functions to create an ImageOutputHandle that
 * examines the filename in order to decide what format to use. The following
 * ImageOutputHandle constructors are only needed if you want to specify
 * yourself what format to use. */
extern ImageOutputHandle *CreatePPMOutputHandle(FILE *fp, int width, int height);
extern ImageOutputHandle *CreateTiffOutputHandle(char *filename, int width, int height,
                                                 int high_dynamic_range, float ref_lum);

/* ---------------------------------------------------------------------------
  `ImageFileExtension'

  Returns file name extension. Understands extra suffixes ".Z", ".gz",
  ".bz", and ".bz2".
  ------------------------------------------------------------------------- */
extern char *ImageFileExtension(char *fname);

/* Write a scanline of display RGB, RGB radiance or CIE XYZ radiance data.
 * 3 samples per pixel: RGB order for RGB data and XYZ order for CIE XYZ data */
extern int WriteDisplayRGB(ImageOutputHandle *handle, unsigned char *data);

/* finish writing the image */
extern void DeleteImageOutputHandle(ImageOutputHandle *handle);

#endif
