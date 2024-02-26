#ifndef _INTERACTIONLIST_H_
#define _INTERACTIONLIST_H_

#include "GALERKIN/Interaction.h"
#include "common/dataStructures/List.h"

class Interaction;

class INTERACTIONLIST {
  public:
    Interaction *interaction;
    INTERACTIONLIST *next;
};

#define InteractionListAdd(interactionlist, interaction) \
        (INTERACTIONLIST *)ListAdd((LIST *)interactionlist, (void *)interaction)

#define InteractionListRemove(interactionlist, interaction) \
        (INTERACTIONLIST *)ListRemove((LIST *)interactionlist, (void *)interaction)

#define InteractionListIterate(interactionlist, proc) \
        ListIterate((LIST *)interactionlist, (void (*)(void *))proc)

#define InteractionListDestroy(interactionlist) \
        ListDestroy((LIST *)interactionlist)

#endif
