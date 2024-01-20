#ifndef __BREP_EDGE__
#define __BREP_EDGE__

#include "BREP/BREP_WING.h"

class BREP_EDGE {
  public:
    void *client_data;
    BREP_WING wing[2]; // Two wings
};

extern void brepDestroyEdge(BREP_EDGE *edge);

#endif
