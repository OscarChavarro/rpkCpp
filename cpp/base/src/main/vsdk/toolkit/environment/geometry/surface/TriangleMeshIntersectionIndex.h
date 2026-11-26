#ifndef __TRIANGLE_MESH_INTERSECTION_INDEX__
#define __TRIANGLE_MESH_INTERSECTION_INDEX__

#include "java/util/concurrent/atomic/AtomicLong.h"

// Legacy last-hit index; queries carrying their own index must not depend on it.
class TriangleMeshIntersectionIndex {
private:
    java::AtomicLong value;
public:
    TriangleMeshIntersectionIndex();
    TriangleMeshIntersectionIndex(const TriangleMeshIntersectionIndex& other);
    TriangleMeshIntersectionIndex& operator=(const TriangleMeshIntersectionIndex& other);
    int get() const;
    void set(int index);
};
#endif
