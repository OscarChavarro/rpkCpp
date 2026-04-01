#ifndef __PIC_OUTPUT_HANDLE__
#define __PIC_OUTPUT_HANDLE__

#include "java/io/OutputStream.h"
#include "java/lang/String.h"
#include <cstdarg>
#include "io/image/ImageOutputHandle.h"

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
class PicOutputHandle final : public ImageOutputHandle {
  private:
    java::OutputStream *outputStream;

    static java::String formatToString(const char *format, va_list arguments);
    static void writeFormatted(java::OutputStream *outputStream, const char *format, ...);
    void writeHeader();

  public:
    PicOutputHandle(const char *filename, int w, int h);
    ~PicOutputHandle() final;
    int writeRadianceRGB(ColorRgb *rgbRadiance) final;
};

#endif
