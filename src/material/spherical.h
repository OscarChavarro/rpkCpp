/**
Routine for sampling directions on a spherical triangle or
quadrilateral using Arvo's technique published in SIGGRAPH '95 p 437
and similar stuff
*/

#ifndef __SPHERICAL__
#define __SPHERICAL__

#include "skin/Patch.h"

class CoordSys {
  public:
    Vector3D X;
    Vector3D Y;
    Vector3D Z;
};

extern void patchCoordSys(Patch *P, CoordSys *coord);
extern void vectorCoordSys(Vector3D *Z, CoordSys *coord);
extern void vectorToSphericalCoord(Vector3D *C, CoordSys *coordSys, double *phi, double *theta);
extern Vector3D sampleHemisphereCosTheta(CoordSys *coord, double xi_1, double xi_2, double *probabilityDensityFunction);
extern Vector3D sampleHemisphereCosNTheta(CoordSys *coord, double n, double xi_1, double xi_2, double *probabilityDensityFunction);

#endif

