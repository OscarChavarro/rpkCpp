/* elementlist.h: linear lists of ELEMENTs */

#ifndef _ELEMENTLIST_H_
#define _ELEMENTLIST_H_

#include "GALERKIN/GalerkinElement.h"
#include "common/dataStructures/List.h"

class GalerkingElement;

/* same layout as LIST in dataStructures/List.h in order to be able to use
 * the generic list procedures defined in dataStructures/List.c */
class ELEMENTLIST {
  public:
    GalerkingElement *element;
    ELEMENTLIST *next;
};

#define ElementListCreate    (ELEMENTLIST *)ListCreate

#define ElementListAdd(elementlist, element)    \
        (ELEMENTLIST *)ListAdd((LIST *)elementlist, (void *)element)

#define ElementListIterate(elementlist, proc) \
        ListIterate((LIST *)elementlist, (void (*)(void *))proc)

#define ElementListDestroy(elementlist) \
        ListDestroy((LIST *)elementlist)

#endif
