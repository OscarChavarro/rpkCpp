#ifndef __FILE_UNCOMPRESS_WRAPPER__
#define __FILE_UNCOMPRESS_WRAPPER__

#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"

extern java::io::InputStream *openInputStreamCompressWrapper(const char *fileName, int *isPipe);
extern java::io::OutputStream *openOutputStreamCompressWrapper(const char *fileName, int *isPipe);
extern void closeInputStream(java::io::InputStream *stream);
extern void closeOutputStream(java::io::OutputStream *stream);

#endif
