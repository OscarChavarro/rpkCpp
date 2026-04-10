#include "java/io/InputStream.h"


void
InputStream::dispose() {
    close();
}

InputStream::~InputStream() {}

