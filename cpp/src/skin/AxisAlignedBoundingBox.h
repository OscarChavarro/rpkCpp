#ifndef __BOUNDING_BOX__
#define __BOUNDING_BOX__

#include "common/linealAlgebra/Vector3D.h"
#include "skin/BoundingBoxCoordinateIndex.h"

/**
A bounding box is represented as an array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X
*/
class AxisAlignedBoundingBox {
  private:
    float coordinates[6];

    static void inline
    setIfLess(float &a, float b) {
        a = a < b ? a : b;
    }

    static void inline
    setIfGreater(float &a, float b) {
        a = a > b ? a : b;
    }

  public:
    AxisAlignedBoundingBox();

    float maxExtent() const;
    bool outOfBounds(const Vector3D *p) const;
    Vector3D center() const;
    void setAsUnion(const AxisAlignedBoundingBox *a, const AxisAlignedBoundingBox *b);
    bool disjointToOtherBoundingBox(const AxisAlignedBoundingBox *other) const;
    bool behindPlane(const Vector3D *normal, float distance) const;
    void copyFrom(const AxisAlignedBoundingBox *other);
    void enlarge(const AxisAlignedBoundingBox *other);
    void enlargeToIncludePoint(const Vector3D *point);
    void enlargeTinyBit();
    void computeContributionFlags(const AxisAlignedBoundingBox *other, bool *hasMinMax1, bool *hasMinMax2) const;
    float dx() const;
    float dy() const;
    float dz() const;
    Vector3D voxelSize(const short nx, const short ny, const short nz) const;
    void enlargeByFactor(float factor);
    Vector3D minPoint() const;
    Vector3D maxPoint() const;
    float minX() const;
    float minY() const;
    float minZ() const;
    float maxX() const;
    float maxY() const;
    float maxZ() const;
    const float *rawCoordinates() const;
    float valueAt(int idx) const;
    void corners(Vector3D out[8]) const;
};

inline float
AxisAlignedBoundingBox::maxExtent() const {
    const float dx = coordinates[MAX_X] - coordinates[MIN_X];
    const float dy = coordinates[MAX_Y] - coordinates[MIN_Y];
    const float dz = coordinates[MAX_Z] - coordinates[MIN_Z];
    return dx > dy
           ? (dx > dz ? dx : dz)
           : (dy > dz ? dy : dz);
}

inline bool
AxisAlignedBoundingBox::outOfBounds(const Vector3D *p) const {
    return p->x < coordinates[MIN_X] || p->x > coordinates[MAX_X] ||
           p->y < coordinates[MIN_Y] || p->y > coordinates[MAX_Y] ||
           p->z < coordinates[MIN_Z] || p->z > coordinates[MAX_Z];
}

inline Vector3D
AxisAlignedBoundingBox::center() const {
    return Vector3D(
        0.5f * (coordinates[MIN_X] + coordinates[MAX_X]),
        0.5f * (coordinates[MIN_Y] + coordinates[MAX_Y]),
        0.5f * (coordinates[MIN_Z] + coordinates[MAX_Z])
    );
}

inline void
AxisAlignedBoundingBox::setAsUnion(const AxisAlignedBoundingBox *a, const AxisAlignedBoundingBox *b) {
    for (int i = MIN_X; i <= MIN_Z; i++) {
        coordinates[i] = a->coordinates[i] < b->coordinates[i]
                         ? a->coordinates[i]
                         : b->coordinates[i];
    }

    for (int i = MAX_X; i <= MAX_Z; i++) {
        coordinates[i] = a->coordinates[i] > b->coordinates[i]
                         ? a->coordinates[i]
                         : b->coordinates[i];
    }
}

/**
True if the two given bounding boxes are disjoint
*/
inline bool
AxisAlignedBoundingBox::disjointToOtherBoundingBox(const AxisAlignedBoundingBox *other) const {
    return
            (coordinates[MIN_X] > other->coordinates[MAX_X]) || (other->coordinates[MIN_X] > coordinates[MAX_X]) ||
            (coordinates[MIN_Y] > other->coordinates[MAX_Y]) || (other->coordinates[MIN_Y] > coordinates[MAX_Y]) ||
            (coordinates[MIN_Z] > other->coordinates[MAX_Z]) || (other->coordinates[MIN_Z] > coordinates[MAX_Z]);
}

inline void
AxisAlignedBoundingBox::computeContributionFlags(const AxisAlignedBoundingBox *other, bool *hasMinMax1, bool *hasMinMax2) const {
    for (int i = MIN_X; i <= MIN_Z; i++) {
        if ( coordinates[i] < other->coordinates[i]) {
            hasMinMax1[i] = true;
        } else {
            if (!Numeric::doubleEqual(
                    coordinates[i],
                    other->coordinates[i],
                    Numeric::EPSILON)) {
                hasMinMax2[i] = true;
            }
        }
    }

    for (int i = MAX_X; i <= MAX_Z; i++) {
        if ( coordinates[i] > other->coordinates[i]) {
            hasMinMax1[i] = true;
        } else {
            if (!Numeric::doubleEqual(
                    coordinates[i],
                    other->coordinates[i],
                    Numeric::EPSILON)) {
                hasMinMax2[i] = true;
            }
        }
    }
}

inline float
AxisAlignedBoundingBox::dx() const {
    return coordinates[MAX_X] - coordinates[MIN_X];
}

inline float
AxisAlignedBoundingBox::dy() const {
    return coordinates[MAX_Y] - coordinates[MIN_Y];
}

inline float
AxisAlignedBoundingBox::dz() const {
    return coordinates[MAX_Z] - coordinates[MIN_Z];
}

inline Vector3D
AxisAlignedBoundingBox::voxelSize(const short nx, const short ny, const short nz) const {
    return Vector3D(
            dx() / static_cast<float>(nx),
            dy() / static_cast<float>(ny),
            dz() / static_cast<float>(nz)
    );
}

inline void
AxisAlignedBoundingBox::enlargeByFactor(float factor) {
    float fdx = dx() * factor;
    float fdy = dy() * factor;
    float fdz = dz() * factor;

    if (fdx < Numeric::EPSILON_FLOAT) {
        fdx = Numeric::EPSILON_FLOAT;
    }
    if (fdy < Numeric::EPSILON_FLOAT) {
        fdy = Numeric::EPSILON_FLOAT;
    }
    if (fdz < Numeric::EPSILON_FLOAT) {
        fdz = Numeric::EPSILON_FLOAT;
    }

    coordinates[MIN_X] -= fdx;
    coordinates[MAX_X] += fdx;
    coordinates[MIN_Y] -= fdy;
    coordinates[MAX_Y] += fdy;
    coordinates[MIN_Z] -= fdz;
    coordinates[MAX_Z] += fdz;
}

inline Vector3D
AxisAlignedBoundingBox::minPoint() const {
    return Vector3D(
            coordinates[MIN_X],
            coordinates[MIN_Y],
            coordinates[MIN_Z]
    );
}

inline Vector3D
AxisAlignedBoundingBox::maxPoint() const {
    return Vector3D(
            coordinates[MAX_X],
            coordinates[MAX_Y],
            coordinates[MAX_Z]
    );
}

inline float
AxisAlignedBoundingBox::minX() const {
    return coordinates[MIN_X];
}

inline float
AxisAlignedBoundingBox::minY() const {
    return coordinates[MIN_Y];
}

inline float
AxisAlignedBoundingBox::minZ() const {
    return coordinates[MIN_Z];
}

inline float
AxisAlignedBoundingBox::maxX() const {
    return coordinates[MAX_X];
}

inline float
AxisAlignedBoundingBox::maxY() const {
    return coordinates[MAX_Y];
}

inline float
AxisAlignedBoundingBox::maxZ() const {
    return coordinates[MAX_Z];
}

inline const float *
AxisAlignedBoundingBox::rawCoordinates() const {
    return coordinates;
}

inline float
AxisAlignedBoundingBox::valueAt(int idx) const {
    switch (idx) {
        case MIN_X: return minX();
        case MIN_Y: return minY();
        case MIN_Z: return minZ();
        case MAX_X: return maxX();
        case MAX_Y: return maxY();
        case MAX_Z: return maxZ();
        default: return 0.0f;
    }
}

inline void
AxisAlignedBoundingBox::corners(Vector3D out[8]) const {
    const float minX = coordinates[MIN_X];
    const float minY = coordinates[MIN_Y];
    const float minZ = coordinates[MIN_Z];
    const float maxX = coordinates[MAX_X];
    const float maxY = coordinates[MAX_Y];
    const float maxZ = coordinates[MAX_Z];

    out[0].set(minX, minY, minZ);
    out[1].set(maxX, minY, minZ);
    out[2].set(minX, maxY, minZ);
    out[3].set(maxX, maxY, minZ);
    out[4].set(minX, minY, maxZ);
    out[5].set(maxX, minY, maxZ);
    out[6].set(minX, maxY, maxZ);
    out[7].set(maxX, maxY, maxZ);
}

#endif
