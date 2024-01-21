/**
Doubly linked list of HITRECs
*/

#ifndef __HIT_LIST__
#define __HIT_LIST__

#include "material/hit.h"
#include "common/dataStructures/DList.h"

class HITLIST {
  public:
    RayHit *hit;
    HITLIST *prev;
    HITLIST *next;
};

#define HitListAdd(hitlist, hitp) (HITLIST *)DListAdd((DLIST *)hitlist, (void *)hitp)

extern RayHit *duplicateHit(RayHit *hit);

#endif
