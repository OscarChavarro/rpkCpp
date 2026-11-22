#include "java/lang/Math.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/io/mgf/MgfTessellationMath.h"

void
MgfTessellationMath::formatFloat(char *target, int targetLength, double value) {
    java::Formatter::format(target, targetLength, "%.12g", value);
}

/**
Compute u and v given w (normalized)
*/
void
MgfTessellationMath::mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon)
{
    double vArr[3] = {0.0, 0.0, 0.0};
    double wArr[3] = {w->x(), w->y(), w->z()};

    int i;
    for ( i = 0; i < 3; i++ ) {
        if ( wArr[i] > -0.6 && wArr[i] < 0.6 ) {
            break;
        }
    }

    if ( i < 3 ) {
        vArr[i] = 1.0;
    }

    *v = Vector3Dd(vArr[0], vArr[1], vArr[2]);

    *u = v->crossProduct(*w);
    normalizeAndGivePreviousNorm(u, epsilon);
    *v = w->crossProduct(*u);
}

/**
Normalize a vector, return old magnitude
*/
double
MgfTessellationMath::normalizeAndGivePreviousNorm(Vector3Dd *vector, double epsilon)
{
    double len = vector->dotProduct(*vector); // Note: this starts being length ^ 2

    if ( len <= 0.0 ) {
        return 0.0;
    }

    if ( len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
        // First order approximation
        len = 0.5 + 0.5 * len;
    } else {
        len = java::Math::sqrt(len);
    }

    *vector = Vector3Dd(vector->x() / len, vector->y() / len, vector->z() / len);

    return len;
}

bool
MgfTessellationMath::isNull(const Vector3Dd &vector, double epsilon) {
    return vector.dotProduct(vector) <= epsilon * epsilon;
}

/**
Returns squared distance between the two vectors
*/
double
MgfTessellationMath::distanceSquared(const Vector3Dd &a, const Vector3Dd &b) {
    const double dx = b.x() - a.x();
    const double dy = b.y() - a.y();
    const double dz = b.z() - a.z();
    return dx * dx + dy * dy + dz * dz;
}
