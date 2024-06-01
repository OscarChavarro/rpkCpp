#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector3D.h"

const int X_GREATER_MASK = 0x01;
const int Y_GREATER_MASK = 0x02;
const int Z_GREATER_MASK = 0x04;
const int XYZ_EQUAL_MASK = 0x08;

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
    indexValue = java::Math::max(anorm.x, (float)indexValue);

    if ( indexValue == anorm.x ) {
        return CoordinateAxis::X;
    } else {
        return indexValue == anorm.y ? CoordinateAxis::Y : CoordinateAxis::Z;
    }
}

int
Vector3D::compareByDimensions(const Vector3D *other, float epsilon) const {
    int code = 0;

    if ( x > other->x + epsilon ) {
        code += X_GREATER_MASK;
    }
    if ( y > other->y + epsilon ) {
        code += Y_GREATER_MASK;
    }
    if ( z > other->z + epsilon ) {
        code += Z_GREATER_MASK;
    }
    if ( code != 0 ) {
        return code;
    }

    if ( x < other->x - epsilon || y < other->y - epsilon || z < other->z - epsilon ) {
        // Not the same coordinates
        return code;
    }

    // Same coordinates
    return XYZ_EQUAL_MASK;
}

void
Vector3D::print(FILE *fp) const {
    fprintf(fp, "%g %g %g", x, y, z);
}
