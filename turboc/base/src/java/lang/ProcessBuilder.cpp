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
    return ((void *)(popen(commandLine, mode)));
}

int
ProcessBuilder::close(void *processHandle) {
    FILE *handle = ((FILE *)(processHandle));
    if ( handle == NULL ) {
        return -1;
    }
    return pclose(handle);
}

