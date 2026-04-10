#ifndef __POTENTIAL_STATISTICS__
#define __POTENTIAL_STATISTICS__

#include "common/VSDK.h"

class PotentialStatistics {
  public:
    double averageDirectPotential;
    double maxDirectPotential;
    double maxDirectImportance; // Potential times area
    double totalDirectPotential;

    PotentialStatistics();
    void reset();
};

#endif
