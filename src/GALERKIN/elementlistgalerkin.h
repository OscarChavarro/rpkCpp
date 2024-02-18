/* elementlist.h: linear lists of ELEMENTs */

#ifndef __ELEMENT_LIST__
#define __ELEMENT_LIST__

#include "GALERKIN/GalerkinElement.h"
#include "common/dataStructures/List.h"

class GalerkinElement;

/* same layout as LIST in dataStructures/List.h in order to be able to use
 * the generic list procedures defined in dataStructures/List.c */
class StochasticRadiosityElementListNode {
  public:
    GalerkinElement *element;
    StochasticRadiosityElementListNode *next;
};

#define ElementListCreate (ELEMENTLIST *)ListCreate

#define StochasticRadiosityElementListAdd(elementlist, element) \
    (StochasticRadiosityElementListNode *)ListAdd((LIST *)elementlist, (void *)element)

#define StochasticRadiosityElementListIterate(elementlist, proc) \
    ListIterate((LIST *)elementlist, (void (*)(void *))proc)

#define StochasticRadiosityElementListDestroy(elementlist) \
    ListDestroy((LIST *)elementlist)

#endif
