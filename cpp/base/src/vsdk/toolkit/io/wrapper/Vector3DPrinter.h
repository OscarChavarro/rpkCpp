#ifndef VECTOR_3D_PRINTER__
#define VECTOR_3D_PRINTER__

#include "vsdk/toolkit/java/io/PrintStream.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"

class Vector3DPrinter {
  public:
    static void print(const Vector3D &vector, java::PrintStream *stream);
};

#endif
