#ifndef __MATRIX_4X4D__
#define __MATRIX_4X4D__

#include "common/linealAlgebra/Vector3Dd.h"

typedef double MATRIX4Dd[4][4];

extern void copyMat4(double (*m4a)[4], MATRIX4Dd m4b);
extern void multiplyV3(VECTOR3Dd *v3a, const VECTOR3Dd *v3b, const double (*m4)[4]);
extern void multiplyP3(VECTOR3Dd *p3a, const VECTOR3Dd *p3b, const double (*m4)[4]);
extern void setIdent4(double (*m4a)[4]);
extern void multiplyMatrix4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4]);

#endif
