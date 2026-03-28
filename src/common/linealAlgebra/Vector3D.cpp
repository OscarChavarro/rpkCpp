#include "common/linealAlgebra/Vector3D.h"

/**
Find the "dominant" part of the vector (eg patch-normal).
This is used to turn the point-in-polygon test into a 2D problem.
*/
CoordinateAxis
Vector3D::dominantCoordinate() const {
    Vector3D anorm;

    anorm.x = java::Math::abs(x);
    anorm.y = java::Math::abs(y);
    anorm.z = java::Math::abs(z);
    double indexValue = java::Math::max(anorm.y, anorm.z);
    indexValue = java::Math::max(anorm.x, static_cast<float>(indexValue));

    if ( indexValue == anorm.x ) {
        return CoordinateAxis::X;
    } else {
        return indexValue == anorm.y ? CoordinateAxis::Y : CoordinateAxis::Z;
    }
}

void
Vector3D::print(java::io::PrintStream *stream) const {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("%g %g %g", x, y, z);
}
