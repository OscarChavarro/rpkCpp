/**
Doubly linked list of HITRECs
*/

#ifndef _HITLIST_H_
#define _HITLIST_H_

#include "material/hit.h"

#include "common/dataStructures/DList.h"

class HITLIST {
  public:
    HITREC *hit;
    HITLIST *prev;
    HITLIST *next;
};

#define HitListAdd(hitlist, hitp) (HITLIST *)DListAdd((DLIST *)hitlist, (void *)hitp)

extern HITREC *DuplicateHit(HITREC *hit);

#endif
