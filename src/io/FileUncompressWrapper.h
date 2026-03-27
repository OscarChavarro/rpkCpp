#ifndef __FILE_UNCOMPRESS_WRAPPER__
#define __FILE_UNCOMPRESS_WRAPPER__

#include <cstdio>

namespace java {
namespace io {
class FileOutputStream;
}
}

extern FILE *openFileCompressWrapper(const char *fileName, const char *open_mode, int *isPipe);
extern void closeFile(FILE *fp, int isPipe);
extern bool openCompressedOutputStream(const char *fileName, java::io::FileOutputStream &outputStream);

#endif
