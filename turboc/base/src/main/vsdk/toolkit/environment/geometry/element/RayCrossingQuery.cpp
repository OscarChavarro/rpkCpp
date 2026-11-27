#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"

RayCrossingQuery::RayCrossingQuery(double minT, double maxT,
    double minimumChordSquared, double minimumExteriorApproach)
    : minT(minT), maxT(maxT), minimumChordSquared(minimumChordSquared),
      minimumExteriorApproach(minimumExteriorApproach)
{
}
