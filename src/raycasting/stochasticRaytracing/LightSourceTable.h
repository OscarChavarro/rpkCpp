#ifndef __LIGHT_SOURCE_TABLE__
#define __LIGHT_SOURCE_TABLE__

#include "skin/Patch.h"

class LightSourceTable {
  public:
    Patch *patch;
    double flux;

    LightSourceTable();
    LightSourceTable(Patch *patch, double flux);
};

#endif
