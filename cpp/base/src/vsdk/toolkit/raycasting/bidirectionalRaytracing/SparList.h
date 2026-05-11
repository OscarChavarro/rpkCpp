#ifndef SPAR_LIST__
#define SPAR_LIST__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/dataStructures/CircularList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/SparConfig.h"

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
