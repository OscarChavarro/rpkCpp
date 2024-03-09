#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class PatchSet : public Geometry {
  public:
    java::ArrayList<Patch *> *patchList;
    explicit PatchSet(java::ArrayList<Patch *> *input);
    virtual ~PatchSet();
};

extern BoundingBox *
patchListBounds(java::ArrayList<Patch *> *patchList, BoundingBox *boundingBox);

extern RayHit *
patchListIntersect(
    java::ArrayList<Patch *> *patchList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

#endif
