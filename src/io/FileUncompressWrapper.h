#ifndef _RPK_FILE_H_
#define _RPK_FILE_H_

#include <cstdio>

extern FILE *openFile(const char *filename, const char *open_mode, int *isPipe);
extern void closeFile(FILE *fp, int isPipe);

#endif
