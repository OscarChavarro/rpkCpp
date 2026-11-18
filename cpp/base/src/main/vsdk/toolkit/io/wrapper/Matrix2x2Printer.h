#ifndef __MATRIX_2_X_2_PRINTER__
#define __MATRIX_2_X_2_PRINTER__

#include "java/io/PrintStream.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix2x2.h"
class Matrix2x2Printer {
  public:
    static void print(const Matrix2x2 &matrix, java::PrintStream *stream);
};

#endif
