#ifndef __FILE_UNCOMPRESS_WRAPPER__
#define __FILE_UNCOMPRESS_WRAPPER__

#include <cstdio>

extern FILE *openFileCompressWrapper(const char *fileName, const char *open_mode, int *isPipe);
extern void closeFile(FILE *fp, int isPipe);

#endif
