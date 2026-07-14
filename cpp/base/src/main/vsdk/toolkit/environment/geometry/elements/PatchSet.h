#ifndef PATCH_LIST__
#define PATCH_LIST__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/Geometry.h"

class PatchSet final : public Geometry {
  private:
    java::ArrayList<Patch *> *patchList;
    bool memoryPoolManaged;

  public:
    explicit PatchSet(const java::ArrayList<Patch *> *input);
    ~PatchSet() final;

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const final;

    java::ArrayList<Patch *> *getPatchList() const;
    bool isMemoryPoolManaged() const;
    void setMemoryPoolManaged(bool value);
};

#endif
