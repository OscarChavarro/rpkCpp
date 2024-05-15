/**
Numerical cubature rules needed to compute form factors
*/

#ifndef __CUBATURE_RULE__
#define __CUBATURE_RULE__

#include "common/numericalAnalysis/CubatureDegree.h"

// No rule has more than 20 nodes
#define CUBATURE_MAXIMUM_NODES 20

/**
Note that <u[i], v[i], t[i]> where 0 <= i < numberOfNodes are points that samples valid positions
inside modelled patch geometry (t[i] = 0 for 2D elements). This is used on form factor visibility
computations.
*/
class CubatureRule {
  public:
    const char *description; // Description of the rule
    int numberOfNodes;
    double u[CUBATURE_MAXIMUM_NODES]; // Abscissa (u, v, [t]) and weights w
    double v[CUBATURE_MAXIMUM_NODES];
    double t[CUBATURE_MAXIMUM_NODES];
    double w[CUBATURE_MAXIMUM_NODES];
};

#endif
