/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef _RENDERPARK_ELEMENTP_H_
#define _RENDERPARK_ELEMENTP_H_

#include "skin/vectorlist.h"
#include "raycasting/stochasticRaytracing/elementtype.h"

enum CLUSTERING_MODE {
    NO_CLUSTERING, ISOTROPIC_CLUSTERING, ORIENTED_CLUSTERING
};

/* a link contains a pointer to the receiver and source elements */
class LINK {
  public:
    ELEMENT *rcv;
    ELEMENT *src;
};

/* a refinement action takes a LINK, performs some refinement action and
 * returns a pointer to the refined link. The parameters (us,vs) and
 * (ur,vr) are transformed to become the parameters of the same point on the 
 * subelement resulting after refinement. */
typedef LINK *(*REFINE_ACTION)(LINK *link,
                               ELEMENT *rcvtop, double *ur, double *vr,
                               ELEMENT *srctop, double *us, double *vs);

/* A refinement oracle evaluates whether or not the given candidate
 * link is admissable for light transport. It returns a refinement action 
 * that will refine the link if necessary. If the link is admissable, it
 * returns the special action 'dontRefine' which does nothing. */
typedef REFINE_ACTION (*ORACLE)(LINK *link);

/* Wellknown power-based refinement oracle (Hanrahan'91, with importance
 * a la Smits'92 if GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven is true) */
extern REFINE_ACTION PowerOracle(LINK *);

/* constructs a toplevel link for given toplevel surface elements
 * rcvtop and srctop: the result is a link between the toplevel
 * cluster containing the whole scene and itself if clustering is
 * enabled. If clustering is not enabled, a link between the
 * given toplevel surface elements is returned. */
extern LINK TopLink(ELEMENT *rcvtop, ELEMENT *srctop);

/* Refines a toplevel link (constructed with TopLink() above). The 
 * returned LINK structure contains pointers the admissable
 * elements and corresponding point coordinates for light transport.
 * rcvtop and srctop are toplevel surface elements containing the 
 * endpoint and origin respectively of a line along which light is to
 * be transported. (ur,vr) and (us,vs) are the uniform parameters of
 * the endpoint and origin on the toplevel surface elements on input.
 * They will be replaced by the point parameters on the admissable elements
 * after refinement. */
extern LINK *Refine(LINK *link,
                    ELEMENT *rcvtop, double *ur, double *vr,
                    ELEMENT *srctop, double *us, double *vs,
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

    ELEMENT *topcluster;       /* top cluster element of element
				      hierarchy */
    Vector3DListNode *coords;
    Vector3DListNode *normals;       /* created during element subdivision */
    Vector3DListNode *texCoords;       /* created during element subdivision */
    VERTEXLIST *vertices;
};

extern ELEM_HIER_STATE hierarchy;

#define DEFAULT_EH_EPSILON                  5e-4
#define DEFAULT_EH_MINAREA                  1e-6
#define DEFAULT_EH_HIERARCHICAL_MESHING     true
#define DEFAULT_EH_CLUSTERING               ORIENTED_CLUSTERING
#define DEFAULT_EH_TVERTEX_ELIMINATION        true

extern void ElementHierarchyDefaults();

extern void ElementHierarchyInit();

extern void ElementHierarchyTerminate();

#endif
