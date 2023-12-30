#ifndef _JACOBIAN_H_
#define _JACOBIAN_H_

/**
Jacobian for a quadrilateral patch is J(u, v) = A + B.u + C.v
*/
class JACOBIAN {
  public:
    float A;
    float B;
    float C;
};

extern JACOBIAN *JacobianCreate(float A, float B, float C);
extern void JacobianDestroy(JACOBIAN *jacobian);

#endif
