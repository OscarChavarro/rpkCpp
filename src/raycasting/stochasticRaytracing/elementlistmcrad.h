#ifndef __ELEMENT_LIST__
#define __ELEMENT_LIST__

#include "common/dataStructures/List.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class StochasticRadiosityElement;

class StochasticRadiosityElementListNode {
  public:
    StochasticRadiosityElement *element;
    StochasticRadiosityElementListNode *next;
};

#define StochasticRadiosityElementListAdd(elementlist, element) \
        (StochasticRadiosityElementListNode *)ListAdd((LIST *)elementlist, (void *)element)

#define StochasticRadiosityElementListDestroy(elementlist) \
        ListDestroy((LIST *)elementlist)

#define ForAllStochasticRadiosityElements(elem, elemlist) ForAllInList(StochasticRadiosityElement, elem, elemlist)

#endif
