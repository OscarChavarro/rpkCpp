/* cubature.h: numerical cubature rules needed to compute formfactors
 * and so on */

#ifndef _CUBATURE_H_
#define _CUBATURE_H_

#define CUBAMAXNODES    20    /* no rule has more than 20 nodes */

class CUBARULE {
  public:
    const char *description;    /* description of the rule */
    int degree,    /* degree */
    nrnodes;    /* nr of nodes */
    double u[CUBAMAXNODES], v[CUBAMAXNODES], t[CUBAMAXNODES],
            w[CUBAMAXNODES];/* abscissae (u,v,[t]) and weights w */
};

extern CUBARULE CRQ1;    /* quads, degree 1,  1 nodes */
extern CUBARULE CRQ2;    /* quads, degree 2,  3 nodes */
extern CUBARULE CRQ3;    /* quads, degree 3,  4 nodes */
extern CUBARULE CRQ4;    /* quads, degree 4,  6 nodes */
extern CUBARULE CRQ5;    /* quads, degree 5,  7 nodes */
extern CUBARULE CRQ6;    /* quads, degree 6, 10 nodes */
extern CUBARULE CRQ7;    /* quads, degree 7, 12 nodes */
extern CUBARULE CRQ8;    /* quads, degree 8, 16 nodes */
extern CUBARULE CRQ9;    /* quads, degree 9, 17 nodes */

extern CUBARULE CRQ3PG;    /* quads, degree 3,  4 nodes product rule */
extern CUBARULE CRQ5PG;    /* quads, degree 5,  9 nodes product rule */
extern CUBARULE CRQ7PG;    /* quads, degree 7, 16 nodes product rule */

extern CUBARULE CRT1;    /* triangles, degree 1,  1 nodes */
extern CUBARULE CRT2;    /* triangles, degree 2,  3 nodes */
extern CUBARULE CRT3;    /* triangles, degree 3,  4 nodes */
extern CUBARULE CRT4;    /* triangles, degree 4,  6 nodes */
extern CUBARULE CRT5;    /* triangles, degree 5,  7 nodes */
/* the cheapest rule of degree 6 for triangles also
has 12 nodes, so use the rule of degree 7 instead */
extern CUBARULE CRT7;    /* triangles, degree 7, 12 nodes */
extern CUBARULE CRT8;    /* triangles, degree 8, 16 nodes */
extern CUBARULE CRT9;    /* triangles, degree 9, 19 nodes */

extern CUBARULE CRV1;    /* boxes, degree 1,  8 nodes (the corners) */

extern CUBARULE CRV3PG;    /* boxes, degree 3,  8 nodes product rule */

/* This routine should be called during initialization of the program: it
 * transforms the rules over [-1,1]^2 to rules over the unit square [0,1]^2,
 * which we use to map to patches. */
extern void FixCubatureRules();

/* after the fixing, the weights for every rule will sum to 1.0, which
 * will allow us to treat parallelipipeda and triangles the same way. */

#endif
