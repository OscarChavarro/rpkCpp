#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"

RaySolidClassification::RaySolidClassification() {}

void RaySolidClassification::clear()
{
    boundaries.clear();
    insideSegments.clear();
}

RaySolidClassification::~RaySolidClassification() {}

RaySolidClassification::RaySolidClassification(const RaySolidClassification& other)
    : boundaries(other.boundaries), insideSegments(other.insideSegments) {}

RaySolidClassification& RaySolidClassification::operator=(const RaySolidClassification& other)
{
    // Reuses the existing storage, see RayInstancePath.
    if (this != &other) {
        boundaries.clear();
        for (long i = 0; i < other.boundaries.size(); i++) boundaries.add(other.boundaries[i]);
        insideSegments.clear();
        for (long i = 0; i < other.insideSegments.size(); i++) insideSegments.add(other.insideSegments[i]);
    }
    return *this;
}
