#include "java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"

RpkAxisAlignedBoundingBox::RpkAxisAlignedBoundingBox(): coordinates() {
    coordinates[MIN_X] = java::Float::MAX_VALUE;
    coordinates[MIN_Y] = java::Float::MAX_VALUE;
    coordinates[MIN_Z] = java::Float::MAX_VALUE;
    coordinates[MAX_X] = -java::Float::MAX_VALUE;
    coordinates[MAX_Y] = -java::Float::MAX_VALUE;
    coordinates[MAX_Z] = -java::Float::MAX_VALUE;
}

/**
Enlarge BoundingBox with other
*/
void
RpkAxisAlignedBoundingBox::enlarge(const RpkAxisAlignedBoundingBox *other) {
    setIfLess(coordinates[MIN_X], other->coordinates[MIN_X]);
    setIfLess(coordinates[MIN_Y], other->coordinates[MIN_Y]);
    setIfLess(coordinates[MIN_Z], other->coordinates[MIN_Z]);
    setIfGreater(coordinates[MAX_X], other->coordinates[MAX_X]);
    setIfGreater(coordinates[MAX_Y], other->coordinates[MAX_Y]);
    setIfGreater(coordinates[MAX_Z], other->coordinates[MAX_Z]);
}

void
RpkAxisAlignedBoundingBox::enlargeToIncludePoint(const Vector3Df *point) {
    setIfLess(coordinates[MIN_X], point->x());
    setIfLess(coordinates[MIN_Y], point->y());
    setIfLess(coordinates[MIN_Z], point->z());
    setIfGreater(coordinates[MAX_X], point->x());
    setIfGreater(coordinates[MAX_Y], point->y());
    setIfGreater(coordinates[MAX_Z], point->z());
}

void
RpkAxisAlignedBoundingBox::copyFrom(const RpkAxisAlignedBoundingBox *other) {
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
RpkAxisAlignedBoundingBox::behindPlane(const Vector3Df &normal, float distance) const {
    Vector3Df P;

    if ( normal.x() > 0.0F ) {
        P = P.withX(coordinates[MAX_X]);
    } else {
        P = P.withX(coordinates[MIN_X]);
    }

    if ( normal.y() > 0.0F ) {
        P = P.withY(coordinates[MAX_Y]);
    } else {
        P = P.withY(coordinates[MIN_Y]);
    }

    if ( normal.z() > 0.0F ) {
        P = P.withZ(coordinates[MAX_Z]);
    } else {
        P = P.withZ(coordinates[MIN_Z]);
    }

    return normal.dotProduct(P) + distance <= 0.0;
}

void
RpkAxisAlignedBoundingBox::enlargeTinyBit() {
    float Dx = static_cast<float>((coordinates[MAX_X] - coordinates[MIN_X]) * 1e-4);
    float Dy = static_cast<float>((coordinates[MAX_Y] - coordinates[MIN_Y]) * 1e-4);
    float Dz = static_cast<float>((coordinates[MAX_Z] - coordinates[MIN_Z]) * 1e-4);
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
