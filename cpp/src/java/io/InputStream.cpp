#include "java/io/InputStream.h"

namespace java {

void
InputStream::dispose() {
    close();
}

InputStream::~InputStream() = default;

}
