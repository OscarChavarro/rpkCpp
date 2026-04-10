#include "common/statistics/PotentialStatistics.h"

PotentialStatistics::PotentialStatistics():
    averageDirectPotential(0.0),
    maxDirectPotential(0.0),
    maxDirectImportance(0.0),
    totalDirectPotential(0.0)
{
}

void
PotentialStatistics::reset() {
    averageDirectPotential = 0.0;
    maxDirectPotential = 0.0;
    maxDirectImportance = 0.0;
    totalDirectPotential = 0.0;
}
