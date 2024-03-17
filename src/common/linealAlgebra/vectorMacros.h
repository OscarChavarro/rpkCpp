#ifndef __VECTOR_FUNCTION__
#define __VECTOR_FUNCTION__

#include "common/linealAlgebra/Vector2D.h"
#include "common/linealAlgebra/Vector3D.h"

/**
Given a vector p in 3D space and an index i, which is X_NORMAL, Y_NORMAL
or Z_NORMAL, projects the vector on the YZ, XZ or XY plane respectively
*/
inline void
vectorProject(Vector2D &r, const Vector3D &p, const int i) {
    switch ( i ) {
        case X_NORMAL:
            r.u = p.y;
            r.v = p.z;
            break;
        case Y_NORMAL:
            r.u = p.x;
            r.v = p.z;
            break;
        case Z_NORMAL:
            r.u = p.x;
            r.v = p.y;
            break;
        default:
            break;
    }
}

#endif
