/**
Routine for sampling directions on a spherical triangle or
quadrilateral using Arvo's technique published in SIGGRAPH '95 p 437
and similar stuff
*/

#ifndef __SPHERICAL__
#define __SPHERICAL__

#include "skin/Patch.h"

class COORDSYS {
  public:
    Vector3D X;
    Vector3D Y;
    Vector3D Z;
};

extern void patchCoordSys(Patch *P, COORDSYS *coord);
extern void vectorCoordSys(Vector3D *Z, COORDSYS *coord);
extern void vectorToSphericalCoord(Vector3D *C, COORDSYS *coordsys, double *phi, double *theta);
extern void sphericalCoordToVector(COORDSYS *coordsys, double *phi, double *theta, Vector3D *C);
extern Vector3D sampleHemisphereCosTheta(COORDSYS *coord, double xi_1, double xi_2, double *pdf_value);
extern Vector3D sampleHemisphereCosNTheta(COORDSYS *coord, double n, double xi_1, double xi_2, double *pdf_value);
extern void getNumberOfDivisions(int samples, int *divs1, int *divs2);

#endif

