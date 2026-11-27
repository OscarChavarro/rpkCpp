#ifndef __PATCH_SET__
#define __PATCH_SET__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class PatchSet: public RpkGeometry{ private:
    ArrayList<Patch *> *patchList;
    bool memoryPoolManaged;

  public:
    explicit PatchSet(const ArrayList<Patch *> *input);
    ~PatchSet();

    RpkRayHit *
    discretizationIntersect( Ray *ray, double minimumDistance, double *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;

    ArrayList<Patch *> *getPatchList() const;
    bool isMemoryPoolManaged() const;
    void setMemoryPoolManaged(bool value);
};

#endif
