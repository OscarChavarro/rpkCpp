/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef _RENDERPARK_ELEMENTP_H_
#define _RENDERPARK_ELEMENTP_H_

#include "java/util/ArrayList.h"
#include "skin/vectorlist.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

enum CLUSTERING_MODE {
    NO_CLUSTERING, ISOTROPIC_CLUSTERING, ORIENTED_CLUSTERING
};

/* a link contains a pointer to the receiver and source elements */
class LINK {
  public:
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
};

/* a refinement action takes a LINK, performs some refinement action and
 * returns a pointer to the refined link. The parameters (us,vs) and
 * (ur,vr) are transformed to become the parameters of the same point on the 
 * subelement resulting after refinement. */
typedef LINK *(*REFINE_ACTION)(LINK *link,
                               StochasticRadiosityElement *rcvtop, double *ur, double *vr,
                               StochasticRadiosityElement *srctop, double *us, double *vs);

/* A refinement oracle evaluates whether or not the given candidate
 * link is admissable for light transport. It returns a refinement action 
 * that will refine the link if necessary. If the link is admissable, it
 * returns the special action 'dontRefine' which does nothing. */
typedef REFINE_ACTION (*ORACLE)(LINK *link);

extern REFINE_ACTION powerOracle(LINK *link);
extern LINK topLink(StochasticRadiosityElement *rcvtop, StochasticRadiosityElement *srctop);
extern LINK *hierarchyRefine(
    LINK *link,
    StochasticRadiosityElement *rcvTop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    ORACLE evaluate_link);

/* global parameters controlling hierarchical refinement */
class ELEM_HIER_STATE {
  public:
    float epsilon;                 /* determines meshing accuracy */
    int do_h_meshing;            /* whether or not to do hierarchical
                                      meshing */
    float maxlinkpow;              /* maximum power transported over a link */
    float minarea;           /* minimum allowed element area */
    long nr_elements;             /* number of elements */
    long nr_clusters;             /* number of clusters */
    int tvertex_elimination;       /* whether or not to do T-vertex
				      elimination for rendering. */
    CLUSTERING_MODE clustering;       /* clustering mode, 0 => no clustering */
    ORACLE oracle;           /* refinement oracle to be used */

    StochasticRadiosityElement *topcluster;       /* top cluster element of element
				      hierarchy */
    Vector3DListNode *coords;
    Vector3DListNode *normals;       /* created during element subdivision */
    Vector3DListNode *texCoords;       /* created during element subdivision */
    java::ArrayList<Vertex *> *vertices;
};

extern ELEM_HIER_STATE GLOBAL_stochasticRaytracing_hierarchy;

#define DEFAULT_EH_EPSILON                  5e-4
#define DEFAULT_EH_MINAREA                  1e-6
#define DEFAULT_EH_HIERARCHICAL_MESHING     true
#define DEFAULT_EH_CLUSTERING               ORIENTED_CLUSTERING
#define DEFAULT_EH_TVERTEX_ELIMINATION        true

#include "java/util/ArrayList.h"

extern void elementHierarchyDefaults();
extern void elementHierarchyInit();
extern void elementHierarchyTerminate(java::ArrayList<Patch *> *scenePatches);

#endif
