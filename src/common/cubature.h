/**
Numerical cubature rules needed to compute form factors
*/

#ifndef __CUBATURE__
#define __CUBATURE__

#define CUBAMAXNODES 20 // no rule has more than 20 nodes

class CUBARULE {
  public:
    const char *description; // description of the rule
    int degree;
    int numberOfNodes;
    double u[CUBAMAXNODES];
    double v[CUBAMAXNODES];
    double t[CUBAMAXNODES];
    double w[CUBAMAXNODES]; // abscissae (u,v,[t]) and weights w
};

extern CUBARULE GLOBAL_crq1; // quads, degree 1,  1 nodes
extern CUBARULE GLOBAL_crq2; // quads, degree 2,  3 nodes
extern CUBARULE GLOBAL_crq3; // quads, degree 3,  4 nodes
extern CUBARULE GLOBAL_crq4; // quads, degree 4,  6 nodes
extern CUBARULE GLOBAL_crq5; // quads, degree 5,  7 nodes
extern CUBARULE GLOBAL_crq6; // quads, degree 6, 10 nodes
extern CUBARULE GLOBAL_crq7; // quads, degree 7, 12 nodes
extern CUBARULE GLOBAL_crq8; // quads, degree 8, 16 nodes
extern CUBARULE GLOBAL_crq9; // quads, degree 9, 17 nodes

extern CUBARULE GLOBAL_crq3pg; // quads, degree 3,  4 nodes product rule
extern CUBARULE GLOBAL_crq5pg; // quads, degree 5,  9 nodes product rule
extern CUBARULE GLOBAL_crq7pg; // quads, degree 7, 16 nodes product rule

extern CUBARULE GLOBAL_crt1; // triangles, degree 1,  1 nodes
extern CUBARULE GLOBAL_crt2; // triangles, degree 2,  3 nodes
extern CUBARULE GLOBAL_crt3; // triangles, degree 3,  4 nodes
extern CUBARULE GLOBAL_crt4; // triangles, degree 4,  6 nodes
extern CUBARULE GLOBAL_crt5; // triangles, degree 5,  7 nodes

// The cheapest rule of degree 6 for triangles also has 12 nodes, so use the rule of degree 7 instead
extern CUBARULE GLOBAL_crt7; // triangles, degree 7, 12 nodes
extern CUBARULE GLOBAL_crt8; // triangles, degree 8, 16 nodes
extern CUBARULE GLOBAL_crt9; // triangles, degree 9, 19 nodes

extern CUBARULE GLOBAL_crv1; // boxes, degree 1,  8 nodes (the corners)

extern CUBARULE GLOBAL_crv3pg; // boxes, degree 3,  8 nodes product rule

/* This routine should be called during initialization of the program: it
 * transforms the rules over [-1,1]^2 to rules over the unit square [0,1]^2,
 * which we use to map to patches. */
extern void fixCubatureRules();

/* after the fixing, the weights for every rule will sum to 1.0, which
 * will allow us to treat parallelipipeda and triangles the same way. */

#endif
