#ifndef LIGHT_LIST_ITERATOR__
#define LIGHT_LIST_ITERATOR__

#include "vsdk/toolkit/common/dataStructures/CircularListIterator.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightInfo.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class LightListIterator {
  private:
    CircularListIterator<LightInfo> iterator;
  public:
    explicit LightListIterator(LightList &list);

    Patch *First(LightList &list);
    Patch *Next();
};

#endif
