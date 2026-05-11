#include "vsdk/toolkit/io/wrapper/Matrix2x2Printer.h"

void
Matrix2x2Printer::print(const Matrix2x2 &matrix, java::PrintStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("\t%f %f    %f\n", matrix.m[0][0], matrix.m[0][1], matrix.t[0]);
    stream->printf("\t%f %f    %f\n", matrix.m[0][1], matrix.m[1][1], matrix.t[1]);
}
