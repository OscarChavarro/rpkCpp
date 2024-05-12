#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector3D.h"

/**
Find the "dominant" part of the vector (eg patch-normal).
This is used to turn the point-in-polygon test into a 2D problem.
*/
int
Vector3D::dominantCoordinate() const {
    Vector3D anorm;

    anorm.x = java::Math::abs(x);
    anorm.y = java::Math::abs(y);
    anorm.z = java::Math::abs(z);
    double indexValue = java::Math::max(anorm.y, anorm.z);
    indexValue = java::Math::max(anorm.x, (float)indexValue);

    if ( indexValue == anorm.x ) {
        return X_NORMAL;
    } else {
        return indexValue == anorm.y ? Y_NORMAL : Z_NORMAL;
    }
}

int
Vector3D::compareByDimensions(const Vector3D *v2, float epsilon) const {
    int code = 0;

    if ( x > v2->x + epsilon ) {
        code += X_GREATER_MASK;
    }
    if ( y > v2->y + epsilon ) {
        code += Y_GREATER_MASK;
    }
    if ( z > v2->z + epsilon ) {
        code += Z_GREATER_MASK;
    }
    if ( code != 0 ) {
        return code;
    }

    if ( x < v2->x - epsilon || y < v2->y - epsilon || z < v2->z - epsilon ) {
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
