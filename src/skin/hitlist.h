#ifndef __HIT_LIST__
#define __HIT_LIST__

#include "common/dataStructures/DList.h"
#include "material/hit.h"

class HITLIST {
  public:
    RayHit *hit;
    HITLIST *next;
};

#define HitListAdd(hitlist, hitp) (HITLIST *)listAdd((DLIST *)hitlist, (void *)hitp)

#endif
