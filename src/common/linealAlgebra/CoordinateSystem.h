/**
Routines used to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437

References:
- [ARVO1995b] "Stratified Sampling of Spherical Triangles", SIGGRAPH 1995
*/

#ifndef __COORDINATE_SYSTEM__
#define __COORDINATE_SYSTEM__

#include "common/linealAlgebra/Vector3D.h"

class CoordinateSystem {
  public:
    Vector3D X;
    Vector3D Y;
    Vector3D Z;

    void setFromZAxis(const Vector3D *inZ);
    void rectangularToSphericalCoord(const Vector3D *C, double *phi, double *theta) const;
    Vector3D sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const;
    Vector3D sampleHemisphereCosNTheta(double n, double xi1, double xi2, double *probabilityDensityFunction) const;
};

#endif
