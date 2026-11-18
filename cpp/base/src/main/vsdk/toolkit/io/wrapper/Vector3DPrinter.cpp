#include "vsdk/toolkit/io/wrapper/Vector3DPrinter.h"
void
Vector3DPrinter::print(const Vector3D &vector, java::PrintStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("%g %g %g", vector.x, vector.y, vector.z);
}
