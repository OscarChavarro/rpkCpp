#ifndef __INTERACTION_LIST__
#define __INTERACTION_LIST__

#include "GALERKIN/Interaction.h"
#include "common/dataStructures/List.h"

class Interaction;

class InteractionListNode {
  public:
    Interaction *interaction;
    InteractionListNode *next;
};

#define InteractionListAdd(interactionList, interaction) \
    (InteractionListNode *)listAdd((LIST *)(interactionList), (void *)(interaction))

#define InteractionListRemove(interactionList, interaction) \
    (InteractionListNode *)listRemove((LIST *)(interactionList), (void *)(interaction))

#endif
