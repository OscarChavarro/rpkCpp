#include "io/wrapper/PipeOutputStream.h"

PipeOutputStream::PipeOutputStream(FILE *fileHandle):
    java::io::FileOutputStream(fileHandle, false),
    pipeHandle(fileHandle)
{
}

PipeOutputStream::~PipeOutputStream() {
    close();
}

void
PipeOutputStream::close() {
    if ( pipeHandle == nullptr ) {
        java::io::FileOutputStream::close();
        return;
    }
    java::io::FileOutputStream::close();
    pclose(pipeHandle);
    pipeHandle = nullptr;
}

void
PipeOutputStream::dispose() {
    close();
}
