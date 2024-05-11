#include "common/linealAlgebra/Vector3D.h"

/**
Find the "dominant" part of the vector (eg patch-normal).  This
is used to turn the point-in-polygon test into a 2D problem.
*/
int
vector3DDominantCoord(const Vector3D *v) {
    Vector3D anorm;

    anorm.x = std::fabs(v->x);
    anorm.y = std::fabs(v->y);
    anorm.z = std::fabs(v->z);
    double indexValue = floatMax(anorm.y, anorm.z);
    indexValue = floatMax(anorm.x, (float)indexValue);

    if ( indexValue == anorm.x ) {
        return X_NORMAL;
    } else {
        return indexValue == anorm.y ? Y_NORMAL : Z_NORMAL;
    }
}

int
vectorCompareByDimensions(const Vector3D *v1, const Vector3D *v2, float epsilon) {
    int code = 0;

    if ( v1->x > v2->x + epsilon ) {
        code += X_GREATER_MASK;
    }
    if ( v1->y > v2->y + epsilon ) {
        code += Y_GREATER_MASK;
    }
    if ( v1->z > v2->z + epsilon ) {
        code += Z_GREATER_MASK;
    }
    if ( code != 0 ) {
        return code;
    }

    if ( v1->x < v2->x - epsilon ||
         v1->y < v2->y - epsilon ||
         v1->z < v2->z - epsilon ) {
        // Not the same coordinates
        return code;
    }

    // Same coordinates
    return XYZ_EQUAL_MASK;
}

void
vector3DPrint(FILE *fp, const Vector3D &v) {
    fprintf(fp, "%g %g %g", v.x, v.y, v.z);
}

void
vector3DDestroy(Vector3D *vector) {
    free(vector);
}
