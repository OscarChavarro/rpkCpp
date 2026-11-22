#include "vsdk/toolkit/io/wrapper/Matrix2x2Printer.h"
void
Matrix2x2Printer::print(const Matrix2x2 &matrix, java::PrintStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("\t%f %f    %f\n", matrix.get(0, 0), matrix.get(0, 1), matrix.getTranslation(0));
    stream->printf("\t%f %f    %f\n", matrix.get(1, 0), matrix.get(1, 1), matrix.getTranslation(1));
}
