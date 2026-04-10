#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class PatchSet: public Geometry{ private:
    ArrayList<Patch *> *patchList;

  public:
    explicit PatchSet(const ArrayList<Patch *> *input);
    ~PatchSet();

    RayHit *
    discretizationIntersect( Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore) const;

    ArrayList<Patch *> *getPatchList() const;
};

#endif
