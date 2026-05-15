#include "java/io/OutputStream.h"


void
OutputStream::flush() {
}

void
OutputStream::dispose() {
    close();
}

OutputStream::~OutputStream() {}

