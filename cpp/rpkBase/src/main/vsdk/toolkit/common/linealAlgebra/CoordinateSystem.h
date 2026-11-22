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

#ifndef COORDINATE_SYSTEM__
#define COORDINATE_SYSTEM__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

class CoordinateSystem {
  private:
    Vector3Df X;
    Vector3Df Y;
    Vector3Df Z;

  public:
    const Vector3Df &getX() const;
    const Vector3Df &getY() const;
    const Vector3Df &getZ() const;
    void setX(const Vector3Df &inX);
    void setY(const Vector3Df &inY);
    void setZ(const Vector3Df &inZ);

    void setFromZAxis(const Vector3Df *inZ);
    void rectangularToSphericalCoord(const Vector3Df *C, double *phi, double *theta) const;
    Vector3Df sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const;
    Vector3Df sampleHemisphereCosNTheta(double n, double xi1, double xi2, double *probabilityDensityFunction) const;
};

inline const Vector3Df &
CoordinateSystem::getX() const {
    return X;
}

inline const Vector3Df &
CoordinateSystem::getY() const {
    return Y;
}

inline const Vector3Df &
CoordinateSystem::getZ() const {
    return Z;
}

inline void
CoordinateSystem::setX(const Vector3Df &inX) {
    X = inX;
}

inline void
CoordinateSystem::setY(const Vector3Df &inY) {
    Y = inY;
}

inline void
CoordinateSystem::setZ(const Vector3Df &inZ) {
    Z = inZ;
}

#endif
