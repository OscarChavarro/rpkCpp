#include "common/linealAlgebra/Vector3D.h"

void
vector3DDestroy(Vector3D *vector) {
    free(vector);
}

/**
Find the "dominant" part of the vector (eg patch-normal).  This
is used to turn the point-in-polygon test into a 2D problem.
*/
int
vector3DDominantCoord(Vector3D *v) {
    Vector3D anorm;
    double indexValue;

    anorm.x = std::fabs(v->x);
    anorm.y = std::fabs(v->y);
    anorm.z = std::fabs(v->z);
    indexValue = floatMax(anorm.y, anorm.z);
    indexValue = floatMax(anorm.x, (float)indexValue);

    return indexValue == anorm.x ? X_NORMAL :
            (indexValue == anorm.y ? Y_NORMAL : Z_NORMAL);
}

int
vectorCompareByDimensions(Vector3D *v1, Vector3D *v2, float epsilon) {
    int code = 0;
    if ( v1->x > v2->x + epsilon ) {
        code += X_GREATER;
    }
    if ( v1->y > v2->y + epsilon ) {
        code += Y_GREATER;
    }
    if ( v1->z > v2->z + epsilon ) {
        code += Z_GREATER;
    }
    if ( code != 0 ) {
        // x1 > x2 || y1 > y2 || z1 > z2
        return code;
    }

    if ( v1->x < v2->x - epsilon ||
         v1->y < v2->y - epsilon ||
         v1->z < v2->z - epsilon ) {
        // Not the same coordinates
        return code;
    }

    // Same coordinates
    return XYZ_EQUAL;
}

void
vector3DPrint(FILE *fp, Vector3D &v) {
    fprintf(fp, "%g %g %g", v.x, v.y, v.z);
}
