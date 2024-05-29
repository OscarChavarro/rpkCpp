#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class PatchSet final : public Geometry {
  private:
    java::ArrayList<Patch *> *patchList;

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

    java::ArrayList<Patch *> *getPatchList() const {
        return patchSetData->patchList;
    }
};

extern BoundingBox *
patchListBounds(const java::ArrayList<Patch *> *patchList, BoundingBox *boundingBox);

#endif
