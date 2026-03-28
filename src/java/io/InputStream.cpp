#include "java/io/InputStream.h"

namespace java {
namespace io {

void
InputStream::dispose() {
    close();
}

InputStream::~InputStream() = default;

}
}
