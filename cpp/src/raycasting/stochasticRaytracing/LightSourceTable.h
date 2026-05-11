#ifndef LIGHT_SOURCE_TABLE__
#define LIGHT_SOURCE_TABLE__

#include "environment/geometry/elements/Patch.h"

class LightSourceTable {
  public:
    Patch *patch;
    double flux;

    LightSourceTable();
    LightSourceTable(Patch *patch, double flux);
};

#endif
