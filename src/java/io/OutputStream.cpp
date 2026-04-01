#include "java/io/OutputStream.h"

namespace java {

void
OutputStream::flush() {
}

void
OutputStream::dispose() {
    close();
}

OutputStream::~OutputStream() = default;

}
