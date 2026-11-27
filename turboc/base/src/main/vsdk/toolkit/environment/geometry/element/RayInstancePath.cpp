#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RayInstancePath.h"
RayInstancePath::RayInstancePath() : surfaceIndex(-1), instances() {}
RayInstancePath::RayInstancePath(const RayInstancePath& other)
    : surfaceIndex(other.surfaceIndex), instances(other.instances) {}
RayInstancePath& RayInstancePath::operator=(const RayInstancePath& other)
{
    // Reuses the existing storage: assignment of query results must not
    // allocate once a workspace has reached its working size.
    if (this != &other) {
        surfaceIndex = other.surfaceIndex;
        instances.clear();
        for (long i = 0; i < other.instances.size(); i++) instances.add(other.instances[i]);
    }
    return *this;
}
RayInstancePath::~RayInstancePath() {}
BOOLEAN RayInstancePath::setSurfaceIndex(long index)
{
    if (index < -1) return FALSE;
    surfaceIndex = index; return TRUE;
}
long RayInstancePath::getSurfaceIndex() const { return surfaceIndex; }
BOOLEAN RayInstancePath::appendInstance(const RayInstanceStep& step)
{ return step.instance != NULL && instances.add(step); }
long RayInstancePath::getInstanceCount() const { return instances.size(); }
BOOLEAN RayInstancePath::getInstance(long index, RayInstanceStep& result) const
{
    if (index < 0 || index >= instances.size()) return FALSE;
    result = instances[index]; return TRUE;
}
void RayInstancePath::clearInstances() { instances.clear(); }
void RayInstancePath::clear() { surfaceIndex = -1; instances.clear(); }
