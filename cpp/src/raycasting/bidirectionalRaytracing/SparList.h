#ifndef __SPAR_LIST__
#define __SPAR_LIST__

#include "common/color/ColorRgb.h"
#include "common/dataStructures/CircularList.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "raycasting/bidirectionalRaytracing/SparConfig.h"

class Spar;

class SparList final : public CircularList<Spar *> {
  public:
    void
    handlePath(
        SparConfig *config,
        BiPath *path,
        ColorRgb *fRad,
        ColorRgb *fBpt);
    ~SparList() final;
};

using CSparListIter = CircularListIterator<Spar *>;

#endif
