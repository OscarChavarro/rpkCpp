#ifndef __SPAR_LIST__
#define __SPAR_LIST__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/dataStructures/CircularList.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BiPath.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/SparConfig.h"

class Spar;

struct SparContribution final {
    ColorRgb radiance;
    ColorRgb bptRadiance;
};

class SparList final : public CircularList<Spar *> {
  public:
    SparContribution
    handlePath(
        SparConfig *config,
        BiPath *path);
    ~SparList() final;
};

using CSparListIter = CircularListIterator<Spar *>;

#endif
