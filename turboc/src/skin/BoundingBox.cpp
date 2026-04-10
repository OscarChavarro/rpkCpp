#include "skin/BoundingBox.h"

BoundingBox::BoundingBox(): coordinates() {
    coordinates[MIN_X] = Numeric::HUGE_FLOAT_VALUE;
    coordinates[MIN_Y] = Numeric::HUGE_FLOAT_VALUE;
    coordinates[MIN_Z] = Numeric::HUGE_FLOAT_VALUE;
    coordinates[MAX_X] = -Numeric::HUGE_FLOAT_VALUE;
    coordinates[MAX_Y] = -Numeric::HUGE_FLOAT_VALUE;
    coordinates[MAX_Z] = -Numeric::HUGE_FLOAT_VALUE;
}

/**
Enlarge BoundingBox with other
*/
void
BoundingBox::enlarge(const BoundingBox *other) {
    setIfLess(coordinates[MIN_X], other->coordinates[MIN_X]);
    setIfLess(coordinates[MIN_Y], other->coordinates[MIN_Y]);
    setIfLess(coordinates[MIN_Z], other->coordinates[MIN_Z]);
    setIfGreater(coordinates[MAX_X], other->coordinates[MAX_X]);
    setIfGreater(coordinates[MAX_Y], other->coordinates[MAX_Y]);
    setIfGreater(coordinates[MAX_Z], other->coordinates[MAX_Z]);
}

void
BoundingBox::enlargeToIncludePoint(const Vector3D *point) {
    setIfLess(coordinates[MIN_X], point->x);
    setIfLess(coordinates[MIN_Y], point->y);
    setIfLess(coordinates[MIN_Z], point->z);
    setIfGreater(coordinates[MAX_X], point->x);
    setIfGreater(coordinates[MAX_Y], point->y);
    setIfGreater(coordinates[MAX_Z], point->z);
}

void
BoundingBox::copyFrom(const BoundingBox *other) {
    coordinates[MIN_X] = other->coordinates[MIN_X];
    coordinates[MIN_Y] = other->coordinates[MIN_Y];
    coordinates[MIN_Z] = other->coordinates[MIN_Z];
    coordinates[MAX_X] = other->coordinates[MAX_X];
    coordinates[MAX_Y] = other->coordinates[MAX_Y];
    coordinates[MAX_Z] = other->coordinates[MAX_Z];
}

/**
Returns true if the bounding box is behind the plane defined by norm and d
see F. Tampieri, "Fast Vertex Radiosity Update", Graphics Gems II, p 303
*/
bool
BoundingBox::behindPlane(const Vector3D *normal, float distance) const {
    Vector3D P;

    if ( normal->x > 0.0f ) {
        P.x = coordinates[MAX_X];
    } else {
        P.x = coordinates[MIN_X];
    }

    if ( normal->y > 0.0f ) {
        P.y = coordinates[MAX_Y];
    } else {
        P.y = coordinates[MIN_Y];
    }

    if ( normal->z > 0.0f ) {
        P.z = coordinates[MAX_Z];
    } else {
        P.z = coordinates[MIN_Z];
    }

    return normal->dotProduct(P) + distance <= 0.0;
}

void
BoundingBox::enlargeTinyBit() {
    float Dx = ((float)((coordinates[MAX_X] - coordinates[MIN_X]) * 1e-4));
    float Dy = ((float)((coordinates[MAX_Y] - coordinates[MIN_Y]) * 1e-4));
    float Dz = ((float)((coordinates[MAX_Z] - coordinates[MIN_Z]) * 1e-4));
    if ( Dx < Numeric::EPSILON_FLOAT ) {
        Dx = Numeric::EPSILON_FLOAT;
    }
    if ( Dy < Numeric::EPSILON_FLOAT ) {
        Dy = Numeric::EPSILON_FLOAT;
    }
    if ( Dz < Numeric::EPSILON_FLOAT ) {
        Dz = Numeric::EPSILON_FLOAT;
    }
    coordinates[MIN_X] -= Dx;
    coordinates[MAX_X] += Dx;
    coordinates[MIN_Y] -= Dy;
    coordinates[MAX_Y] += Dy;
    coordinates[MIN_Z] -= Dz;
    coordinates[MAX_Z] += Dz;
}
