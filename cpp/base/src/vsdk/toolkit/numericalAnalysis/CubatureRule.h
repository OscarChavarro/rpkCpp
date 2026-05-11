/**
Numerical cubature rules needed to compute form factors
*/

#ifndef CUBATURE_RULE__
#define CUBATURE_RULE__

#include "vsdk/toolkit/numericalAnalysis/CubatureDegree.h"

/**
Note that <u[i], v[i], t[i]> where 0 <= i < numberOfNodes are points that samples valid positions
inside modelled patch geometry (t[i] = 0 for 2D elements). This is used on form factor visibility
computations.
*/
class CubatureRule {
  public:
    static constexpr int MAXIMUM_NODES = 20;

    const char *description; // Description of the rule
    int numberOfNodes;
    double u[MAXIMUM_NODES]; // Abscissa (u, v, [t]) and weights w
    double v[MAXIMUM_NODES];
    double t[MAXIMUM_NODES];
    double w[MAXIMUM_NODES];
};

#endif
