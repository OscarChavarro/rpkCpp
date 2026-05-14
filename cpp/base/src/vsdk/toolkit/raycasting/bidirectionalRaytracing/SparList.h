#ifndef SPAR_LIST__
#define SPAR_LIST__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
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
        ColorRgbMutable *fRad,
        ColorRgbMutable *fBpt);
    ~SparList() final;
};

using CSparListIter = CircularListIterator<Spar *>;

#endif
