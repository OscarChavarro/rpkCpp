#include "java/io/OutputStream.h"

namespace java {
namespace io {

void
OutputStream::flush() {
}

void
OutputStream::dispose() {
    close();
}

OutputStream::~OutputStream() = default;

}
}
