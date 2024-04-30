/**
Numerical cubature rules needed to compute form factors
*/

#ifndef __CUBATURE__
#define __CUBATURE__

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
    int degree;
    int numberOfNodes;
    double u[CUBATURE_MAXIMUM_NODES]; // Abscissae (u, v, [t]) and weights w
    double v[CUBATURE_MAXIMUM_NODES];
    double t[CUBATURE_MAXIMUM_NODES];
    double w[CUBATURE_MAXIMUM_NODES];
};

extern CubatureRule GLOBAL_crq8; // quads, degree 8, 16 nodes
extern CubatureRule GLOBAL_crt8; // triangles, degree 8, 16 nodes
extern CubatureRule GLOBAL_crv1; // boxes, degree 1,  8 nodes (the corners)

enum CubatureDegree {
    DEGREE_1,
    DEGREE_2,
    DEGREE_3,
    DEGREE_4,
    DEGREE_5,
    DEGREE_6,
    DEGREE_7,
    DEGREE_8,
    DEGREE_9,
    DEGREE_3_PROD,
    DEGREE_5_PROD,
    DEGREE_7_PROD
};

extern void fixCubatureRules();
extern void setCubatureRules(CubatureRule **triRule, CubatureRule **quadRule, CubatureDegree degree);

/**
After the fixing, the weights for every rule will sum to 1.0, which
will allow us to treat parallelipipeda and triangles the same way
*/

#endif
