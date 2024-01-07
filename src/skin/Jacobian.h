#ifndef __JACOBIAN__
#define __JACOBIAN__

/**
Jacobian for a quadrilateral patch is J(u, v) = A + B.u + C.v
*/
class Jacobian {
  public:
    float A;
    float B;
    float C;
};

extern Jacobian *jacobianCreate(float A, float B, float C);
extern void jacobianDestroy(Jacobian *jacobian);

#endif
