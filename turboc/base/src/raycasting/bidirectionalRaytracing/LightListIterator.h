#ifndef __LIGHT_LIST_ITERATOR__
#define __LIGHT_LIST_ITERATOR__

#include "common/dataStructures/CircularListIterator.h"
#include "raycasting/bidirectionalRaytracing/LightInfo.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "environment/geometry/elements/Patch.h"

class LightListIterator {
  private:
    CircularListIterator<LightInfo> iterator;
  public:
    explicit LightListIterator(LightList &list);

    Patch *First(LightList &list);
    Patch *Next();
};

#endif
