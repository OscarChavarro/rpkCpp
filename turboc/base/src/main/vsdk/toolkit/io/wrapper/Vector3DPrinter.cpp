#include <stddef.h>
#include "vsdk/toolkit/io/wrapper/Vector3DPrinter.h"
void
Vector3DPrinter::print(const Vector3D &vector, PrintStream *stream) {
    if ( stream == NULL ) {
        return;
    }
    stream->printf("%g %g %g", vector.x, vector.y, vector.z);
}
