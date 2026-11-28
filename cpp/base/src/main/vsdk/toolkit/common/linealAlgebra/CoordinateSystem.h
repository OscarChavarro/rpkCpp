/**
Routines used to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437

References:
- [ARVO1995b] "Stratified Sampling of Spherical Triangles", SIGGRAPH 1995

This class implements local-frame and spherical-coordinate primitives after
Section [ARVO1995b].2, especially the orthogonal normalized component operator from
equation (6). Equations (1)-(5) in the paper concern spherical
triangle area inversion and are not directly implemented here.
*/

#ifndef __COORDINATE_SYSTEM__
#define __COORDINATE_SYSTEM__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class CoordinateSystem {
  private:
    Vector3Dd xAxis;
    Vector3Dd yAxis;
    Vector3Dd zAxis;

  public:
    const Vector3Dd &getXAxis() const;
    const Vector3Dd &getYAxis() const;
    const Vector3Dd &getZAxis() const;
    void setXAxis(const Vector3Dd &inX);
    void setYAxis(const Vector3Dd &inY);
    void setZAxis(const Vector3Dd &inZ);

    void setFromZAxis(const Vector3Dd *inZ, double epsilon);
    void rectangularToSphericalCoord(const Vector3Dd *C, double *phi, double *theta, double epsilon) const;
    Vector3Dd sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const;
    Vector3Dd sampleHemisphereCosNTheta(double n, double xi1, double xi2, double *probabilityDensityFunction) const;
};

inline const Vector3Dd &
CoordinateSystem::getXAxis() const {
    return xAxis;
}

inline const Vector3Dd &
CoordinateSystem::getYAxis() const {
    return yAxis;
}

inline const Vector3Dd &
CoordinateSystem::getZAxis() const {
    return zAxis;
}

inline void
CoordinateSystem::setXAxis(const Vector3Dd &inX) {
    xAxis = inX;
}

inline void
CoordinateSystem::setYAxis(const Vector3Dd &inY) {
    yAxis = inY;
}

inline void
CoordinateSystem::setZAxis(const Vector3Dd &inZ) {
    zAxis = inZ;
}

#endif
