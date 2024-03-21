#ifndef __VECTOR3DD__
#define __VECTOR3DD__

#define FLOAT double
#define FLOAT_TINY (1e-6)
#define FLOAT_HUGE (1e10)

// Should be changed to Vector3Dd
typedef FLOAT VECTOR3Dd[3];

typedef FLOAT MATRIX4Dd[4][4];

#define MAT4IDENT { {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, \
                {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} }

extern MATRIX4Dd GLOBAL_mgf_m4Ident;

extern double normalize(double *v);
extern void floatCrossProduct(VECTOR3Dd result, const VECTOR3Dd a, const VECTOR3Dd b);
extern void mgfMakeAxes(double *u, double *v, const double *w);
extern void multiplyP3(double *p3a, double *p3b, double (*m4)[4]);
extern void multiplyV3(double *v3a, const double *v3b, double (*m4)[4]);
extern void multiplyMatrix4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4]);
extern void copyMat4(double (*m4a)[4], MATRIX4Dd m4b);
extern void setIdent4(double (*m4a)[4]);

#endif
