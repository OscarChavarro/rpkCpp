#ifndef __HIT_LIST__
#define __HIT_LIST__

#include "material/hit.h"

class HITLIST {
  public:
    RayHit *hit;
    HITLIST *next;
};

#endif
