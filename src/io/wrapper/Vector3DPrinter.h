#ifndef __VECTOR_3D_PRINTER__
#define __VECTOR_3D_PRINTER__

#include "java/io/PrintStream.h"
#include "common/linealAlgebra/Vector3D.h"

class Vector3DPrinter {
  public:
    static void print(const Vector3D &vector, java::PrintStream *stream);
};

#endif
