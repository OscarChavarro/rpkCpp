#ifndef __RAY_CROSSING_QUERY__
#define __RAY_CROSSING_QUERY__

#include "java/lang/Double.h"

// Interval in the original affine ray parameterization, including its endpoints.
class RayCrossingQuery {
public:
    double minT;
    double maxT;
    double minimumChordSquared;
    // Compatibility cutoff in normalized-direction distance units.
    double minimumExteriorApproach;

    RayCrossingQuery(double minT, double maxT, double minimumChordSquared = 0.0,
        double minimumExteriorApproach = java::Double::NEGATIVE_INFINITY);
};

#endif
