#ifndef _RPK_FILE_H_
#define _RPK_FILE_H_

#include <cstdio>

extern FILE *OpenFile(const char *filename, const char *open_mode, int *ispipe);
extern void CloseFile(FILE *fp, int ispipe);

#endif
