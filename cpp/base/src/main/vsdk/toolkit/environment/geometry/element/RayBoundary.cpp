#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
RayBoundary::RayBoundary(double value, const Geometry* leaf, bool reversed, long index)
    : t(value), geometry(leaf), normalReversed(reversed), surfaceIndex(index), path() {}
RayBoundary::RayBoundary() : t(0), geometry(nullptr), normalReversed(false), surfaceIndex(-1), path() {}
RayBoundary::RayBoundary(const RayBoundary& other)
    : t(other.t), geometry(other.geometry), normalReversed(other.normalReversed), surfaceIndex(other.surfaceIndex), path(other.path) {}
RayBoundary& RayBoundary::operator=(const RayBoundary& other)
{
    if (this != &other) { t = other.t; geometry = other.geometry; normalReversed = other.normalReversed; surfaceIndex = other.surfaceIndex; path = other.path; }
    return *this;
}
RayBoundary::~RayBoundary() {}
RayInstancePath& RayBoundary::getProvenance() { return path; }
const RayInstancePath& RayBoundary::getProvenance() const { return path; }
bool RayBoundary::appendInstance(const RayInstanceStep& step) { return path.appendInstance(step); }
long RayBoundary::getInstanceCount() const { return path.getInstanceCount(); }
bool RayBoundary::getInstance(long index, RayInstanceStep& result) const { return path.getInstance(index, result); }
void RayBoundary::clearInstances() { path.clearInstances(); }
