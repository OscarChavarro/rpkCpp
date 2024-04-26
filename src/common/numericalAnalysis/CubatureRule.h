/**
Numerical cubature rules needed to compute form factors
*/

#ifndef __CUBATURE__
#define __CUBATURE__

#define CUBATURE_MAXIMUM_NODES 20 // no rule has more than 20 nodes

class CubatureRule {
  public:
    const char *description; // description of the rule
    int degree;
    int numberOfNodes;
    double u[CUBATURE_MAXIMUM_NODES];
    double v[CUBATURE_MAXIMUM_NODES];
    double t[CUBATURE_MAXIMUM_NODES];
    double w[CUBATURE_MAXIMUM_NODES]; // abscissae (u,v,[t]) and weights w
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
