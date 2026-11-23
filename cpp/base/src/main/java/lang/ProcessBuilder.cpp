#include <cstdio>

#include "java/lang/ProcessBuilder.h"
namespace java {

ProcessBuilder::ProcessBuilder(const char *commandLine):
    command(commandLine)
{
}

FILE *
ProcessBuilder::startRead() const {
    return start(command, "r");
}

FILE *
ProcessBuilder::startWrite() const {
    return start(command, "w");
}

FILE *
ProcessBuilder::start(const char *commandLine, const char *mode) {
    if ( commandLine == nullptr || commandLine[0] == '\0' || mode == nullptr || mode[0] == '\0' ) {
        return nullptr;
    }
    return popen(commandLine, mode);
}

int
ProcessBuilder::close(FILE *processHandle) {
    FILE *handle = processHandle;
    if ( handle == nullptr ) {
        return -1;
    }
    return pclose(handle);
}

}
