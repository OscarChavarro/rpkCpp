#ifndef __RAY_INSTANCE_PATH__
#define __RAY_INSTANCE_PATH__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RayInstanceStep.h"

// Surface locator and instance transforms, from leaf outward. Identities are borrowed.
class RayInstancePath {
private:
    long surfaceIndex;
    java::ArrayList<RayInstanceStep> instances;
public:
    RayInstancePath();
    RayInstancePath(const RayInstancePath& other);
    RayInstancePath& operator=(const RayInstancePath& other);
    ~RayInstancePath();
    bool setSurfaceIndex(long index);
    long getSurfaceIndex() const;
    bool appendInstance(const RayInstanceStep& step);
    long getInstanceCount() const;
    bool getInstance(long index, RayInstanceStep& result) const;
    void clearInstances();
    void clear();
};
#endif
