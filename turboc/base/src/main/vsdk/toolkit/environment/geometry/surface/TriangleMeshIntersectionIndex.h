#ifndef __TRNGL_MSH_INTRSCTN_INDX__
#define __TRNGL_MSH_INTRSCTN_INDX__

#include "java/util/concurrent/atomic/AtomicLong.h"

// Legacy last-hit index; queries carrying their own index must not depend on it.
class TriangleMeshIntersectionIndex {
private:
    AtomicLong value;
public:
    TriangleMeshIntersectionIndex();
    TriangleMeshIntersectionIndex(const TriangleMeshIntersectionIndex& other);
    TriangleMeshIntersectionIndex& operator=(const TriangleMeshIntersectionIndex& other);
    int get() const;
    void set(int index);
};
#endif
