#ifndef __ELEMENT_LIST__
#define __ELEMENT_LIST__

#include "GALERKIN/GalerkinElement.h"
#include "common/dataStructures/List.h"

/**
Same layout as LIST in dataStructures/List.h in order to be able to use
the generic list procedures defined in dataStructures/List.cpp
*/
class GalerkinElementListNode {
  public:
    GalerkinElement *element;
    GalerkinElementListNode *next;
};

#define GalerkinElementListAdd(elementlist, element) \
    (GalerkinElementListNode *)ListAdd((LIST *)elementlist, (void *)element)

#endif
