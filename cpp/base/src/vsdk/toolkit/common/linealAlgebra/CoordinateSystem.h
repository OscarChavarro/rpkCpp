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

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"

class CoordinateSystem {
  private:
    Vector3D X;
    Vector3D Y;
    Vector3D Z;

  public:
    const Vector3D &getX() const;
    const Vector3D &getY() const;
    const Vector3D &getZ() const;
    void setX(const Vector3D &inX);
    void setY(const Vector3D &inY);
    void setZ(const Vector3D &inZ);

    void setFromZAxis(const Vector3D *inZ);
    void rectangularToSphericalCoord(const Vector3D *C, double *phi, double *theta) const;
    Vector3D sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const;
    Vector3D sampleHemisphereCosNTheta(double n, double xi1, double xi2, double *probabilityDensityFunction) const;
};

inline const Vector3D &
CoordinateSystem::getX() const {
    return X;
}

inline const Vector3D &
CoordinateSystem::getY() const {
    return Y;
}

inline const Vector3D &
CoordinateSystem::getZ() const {
    return Z;
}

inline void
CoordinateSystem::setX(const Vector3D &inX) {
    X = inX;
}

inline void
CoordinateSystem::setY(const Vector3D &inY) {
    Y = inY;
}

inline void
CoordinateSystem::setZ(const Vector3D &inZ) {
    Z = inZ;
}

#endif
