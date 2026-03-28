#include "io/wrapper/PipeInputStream.h"

PipeInputStream::PipeInputStream(FILE *fileHandle):
    java::io::FileInputStream(fileHandle, false),
    pipeHandle(fileHandle)
{
}

PipeInputStream::~PipeInputStream() {
    close();
}

void
PipeInputStream::close() {
    if ( pipeHandle == nullptr ) {
        java::io::FileInputStream::close();
        return;
    }
    java::io::FileInputStream::close();
    pclose(pipeHandle);
    pipeHandle = nullptr;
}

void
PipeInputStream::dispose() {
    close();
}
