#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"

RpkAxisAlignedBoundingBox::RpkAxisAlignedBoundingBox(): coordinates() {
    coordinates[MIN_X] = Double::MAX_VALUE;
    coordinates[MIN_Y] = Double::MAX_VALUE;
    coordinates[MIN_Z] = Double::MAX_VALUE;
    coordinates[MAX_X] = -Double::MAX_VALUE;
    coordinates[MAX_Y] = -Double::MAX_VALUE;
    coordinates[MAX_Z] = -Double::MAX_VALUE;
}

/**
Enlarge RpkAxisAlignedBoundingBox with other
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
RpkAxisAlignedBoundingBox::enlargeToIncludePoint(const Vector3Dd *point) {
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
see F. Tampieri, "Fast RpkVertex Radiosity Update", Graphics Gems II, p 303
*/
bool
RpkAxisAlignedBoundingBox::behindPlane(const Vector3Dd &normal, double distance) const {
    Vector3Dd P;

    if ( normal.x() > 0.0 ) {
        P = P.withX(coordinates[MAX_X]);
    } else {
        P = P.withX(coordinates[MIN_X]);
    }

    if ( normal.y() > 0.0 ) {
        P = P.withY(coordinates[MAX_Y]);
    } else {
        P = P.withY(coordinates[MIN_Y]);
    }

    if ( normal.z() > 0.0 ) {
        P = P.withZ(coordinates[MAX_Z]);
    } else {
        P = P.withZ(coordinates[MIN_Z]);
    }

    return normal.dotProduct(P) + distance <= 0.0;
}

void
RpkAxisAlignedBoundingBox::enlargeTinyBit() {
    double Dx = ((double)((coordinates[MAX_X] - coordinates[MIN_X]) * 1e-4));
    double Dy = ((double)((coordinates[MAX_Y] - coordinates[MIN_Y]) * 1e-4));
    double Dz = ((double)((coordinates[MAX_Z] - coordinates[MIN_Z]) * 1e-4));
    if ( Dx < Numeric::EPSILON ) {
        Dx = Numeric::EPSILON;
    }
    if ( Dy < Numeric::EPSILON ) {
        Dy = Numeric::EPSILON;
    }
    if ( Dz < Numeric::EPSILON ) {
        Dz = Numeric::EPSILON;
    }
    coordinates[MIN_X] -= Dx;
    coordinates[MAX_X] += Dx;
    coordinates[MIN_Y] -= Dy;
    coordinates[MAX_Y] += Dy;
    coordinates[MIN_Z] -= Dz;
    coordinates[MAX_Z] += Dz;
}
