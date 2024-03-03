#ifndef __FILE_UNCOMPRESS_WRAPPER_
#define __FILE_UNCOMPRESS_WRAPPER_

#include <cstdio>

extern FILE *openFile(const char *filename, const char *open_mode, int *isPipe);
extern void closeFile(FILE *fp, int isPipe);

#endif
