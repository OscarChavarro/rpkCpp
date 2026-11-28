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
    Vector3Dd X;
    Vector3Dd Y;
    Vector3Dd Z;

  public:
    const Vector3Dd &getX() const;
    const Vector3Dd &getY() const;
    const Vector3Dd &getZ() const;
    void setX(const Vector3Dd &inX);
    void setY(const Vector3Dd &inY);
    void setZ(const Vector3Dd &inZ);

    void setFromZAxis(const Vector3Dd *inZ, double epsilon);
    void rectangularToSphericalCoord(const Vector3Dd *C, double *phi, double *theta, double epsilon) const;
    Vector3Dd sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const;
    Vector3Dd sampleHemisphereCosNTheta(double n, double xi1, double xi2, double *probabilityDensityFunction) const;
};

inline const Vector3Dd &
CoordinateSystem::getX() const {
    return X;
}

inline const Vector3Dd &
CoordinateSystem::getY() const {
    return Y;
}

inline const Vector3Dd &
CoordinateSystem::getZ() const {
    return Z;
}

inline void
CoordinateSystem::setX(const Vector3Dd &inX) {
    X = inX;
}

inline void
CoordinateSystem::setY(const Vector3Dd &inY) {
    Y = inY;
}

inline void
CoordinateSystem::setZ(const Vector3Dd &inZ) {
    Z = inZ;
}

#endif
