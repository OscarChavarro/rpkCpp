#include "common/linealAlgebra/Matrix2x2.h"

void
Matrix2x2::print(java::io::PrintStream *stream) const {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("\t%f %f    %f\n", m[0][0], m[0][1], t[0]);
    stream->printf("\t%f %f    %f\n", m[0][1], m[1][1], t[1]);
}
