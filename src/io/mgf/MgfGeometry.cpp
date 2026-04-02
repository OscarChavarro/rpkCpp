#include "java/util/Formatter.h"
#include "io/mgf/MgfGeometry.h"

void
MgfGeometry::formatFloat(char *target, int targetLength, double value) {
    java::Formatter::format(target, targetLength, "%.12g", value);
}

/**
Compute u and v given w (normalized)
*/
void
MgfGeometry::mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon)
{
    v->x = 0.0;
    v->y = 0.0;
    v->z = 0.0;
    double vArr[3] = {v->x, v->y, v->z};
    double wArr[3] = {w->x, w->y, w->z};

    int i;
    for ( i = 0; i < 3; i++ ) {
        if ( wArr[i] > -0.6 && wArr[i] < 0.6 ) {
            break;
        }
    }

    if ( i < 3 ) {
        vArr[i] = 1.0;
    }

    v->x = vArr[0];
    v->y = vArr[1];
    v->z = vArr[2];

    u->crossProduct(v, w);
    u->normalizeAndGivePreviousNorm(epsilon);
    v->crossProduct(w, u);
}
