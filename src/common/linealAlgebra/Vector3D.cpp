#include "common/linealAlgebra/vectorMacros.h"

void
VectorDestroy(Vector3D *vector) {
    free((char *)vector);
}

/**
Find the "dominant" part of the vector (eg patch-normal).  This
is used to turn the point-in-polygon test into a 2D problem.
*/
int
VectorDominantCoord(Vector3D *v) {
    Vector3Dd anorm;
    double indexValue;

    anorm.x = std::fabs(v->x);
    anorm.y = std::fabs(v->y);
    anorm.z = std::fabs(v->z);
    indexValue = MAX(anorm.y, anorm.z);
    indexValue = MAX(anorm.x, indexValue);

    return (indexValue == anorm.x ? XNORMAL :
               (indexValue == anorm.y ? YNORMAL : ZNORMAL)
           );
}

int
VectorCompare(Vector3D *v1, Vector3D *v2, float tolerance) {
    int code = 0;
    if ( v1->x > v2->x + tolerance ) {
        code += X_GREATER;
    }
    if ( v1->y > v2->y + tolerance ) {
        code += Y_GREATER;
    }
    if ( v1->z > v2->z + tolerance ) {
        code += Z_GREATER;
    }
    if ( code != 0 ) {
        // x1 > x2 || y1 > y2 || z1 > z2
        return code;
    }

    if ( v1->x < v2->x - tolerance ||
         v1->y < v2->y - tolerance ||
         v1->z < v2->z - tolerance ) {
        // not the same coordinates
        return code;
    }

    // same coordinates
    return XYZ_EQUAL;
}

void
VectorPrint(FILE *fp, Vector3D &v) {
    fprintf(fp, "%g %g %g", v.x, v.y, v.z);
}
