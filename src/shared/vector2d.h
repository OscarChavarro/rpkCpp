#ifndef _VECTOR2D_H_
#define _VECTOR2D_H_

#include "common/mymath.h"
#include "common/linealAlgebra/Vector4D.h"

/* Vector difference */
#define VEC2DSUBTRACT VEC2DDIFF
#define VEC2DDIFF(a, b, o)      {(o).u = (a).u - (b).u; \
                     (o).v = (a).v - (b).v;}

/* Square of vector norm: scalar product with itself */
#define VEC2DNORM2(d)          ((d).u * (d).u + (d).v * (d).v)

#endif
