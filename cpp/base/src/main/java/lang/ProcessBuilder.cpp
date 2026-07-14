#include <cstdio>

#include "java/lang/ProcessBuilder.h"

namespace java {

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
    if ( commandLine == nullptr || commandLine[0] == '\0' || mode == nullptr || mode[0] == '\0' ) {
        return nullptr;
    }
    return static_cast<void *>(popen(commandLine, mode));
}

int
ProcessBuilder::close(void *processHandle) {
    FILE *handle = static_cast<FILE *>(processHandle);
    if ( handle == nullptr ) {
        return -1;
    }
    return pclose(handle);
}

}
