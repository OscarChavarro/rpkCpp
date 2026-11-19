#include <stdio.h>

#include "java/lang/ProcessBuilder.h"

ProcessBuilder::ProcessBuilder(const char *commandLine):
    command(commandLine)
{
}

void *
ProcessBuilder::startRead() const {
    return start(command, "r");
}

void *
ProcessBuilder::startWrite() const {
    return start(command, "w");
}

void *
ProcessBuilder::start(const char *commandLine, const char *mode) {
    if ( commandLine == NULL || commandLine[0] == '\0' || mode == NULL || mode[0] == '\0' ) {
        return NULL;
    }
#ifdef __TURBOC__
    // DOS has no pipes
    return NULL;
#else
    return ((void *)(popen(commandLine, mode)));
#endif
}

int
ProcessBuilder::close(void *processHandle) {
    FILE *handle = ((FILE *)(processHandle));
    if ( handle == NULL ) {
        return -1;
    }
#ifdef __TURBOC__
    return -1;
#else
    return pclose(handle);
#endif
}

