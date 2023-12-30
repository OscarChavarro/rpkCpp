#ifndef _INTERACTIONLIST_H_
#define _INTERACTIONLIST_H_

#include "GALERKIN/interaction.h"
#include "common/dataStructures/List.h"

class INTERACTION;

class INTERACTIONLIST {
  public:
    INTERACTION *interaction;
    INTERACTIONLIST *next;
};

#define InteractionListCreate    (INTERACTIONLIST *)ListCreate

#define InteractionListAdd(interactionlist, interaction)    \
        (INTERACTIONLIST *)ListAdd((LIST *)interactionlist, (void *)interaction)

#define InteractionListRemove(interactionlist, interaction) \
        (INTERACTIONLIST *)ListRemove((LIST *)interactionlist, (void *)interaction)

#define InteractionListIterate(interactionlist, proc) \
        ListIterate((LIST *)interactionlist, (void (*)(void *))proc)

#define InteractionListIterate1B(interactionlist, proc, data) \
        ListIterate1B((LIST *)interactionlist, (void (*)(void *, void *))proc, (void *)data)

#define InteractionListDestroy(interactionlist) \
        ListDestroy((LIST *)interactionlist)

#endif
