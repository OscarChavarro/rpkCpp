#ifndef __MATRIX_4X4D__
#define __MATRIX_4X4D__

typedef double MATRIX4Dd[4][4];

extern void copyMat4(double (*m4a)[4], MATRIX4Dd m4b);
extern void multiplyV3(double *v3a, const double *v3b, double (*m4)[4]);
extern void multiplyP3(double *p3a, const double *p3b, double (*m4)[4]);
extern void setIdent4(double (*m4a)[4]);
extern void multiplyMatrix4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4]);

#endif
