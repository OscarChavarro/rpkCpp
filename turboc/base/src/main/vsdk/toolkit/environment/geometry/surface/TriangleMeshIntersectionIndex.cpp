#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshIntersectionIndex.h"

TriangleMeshIntersectionIndex::TriangleMeshIntersectionIndex() : value(-1) {}
TriangleMeshIntersectionIndex::TriangleMeshIntersectionIndex(const TriangleMeshIntersectionIndex& other) :
    value(other.get()) {}
TriangleMeshIntersectionIndex& TriangleMeshIntersectionIndex::operator=(const TriangleMeshIntersectionIndex& other)
{
    if (this != &other) value.set(other.get());
    return *this;
}
int TriangleMeshIntersectionIndex::get() const { return (int)value.get(); }
void TriangleMeshIntersectionIndex::set(int index)
{
    // Every traced ray stores its result here, from every thread: skipping
    // the write when the value is already there (most often -1, a miss)
    // keeps the shared cache line from bouncing between cores.
    if (value.get() != index) value.set(index);
}
