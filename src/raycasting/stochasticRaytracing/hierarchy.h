/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef __ELEMENT_HIERARCHY__
#define __ELEMENT_HIERARCHY__

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

enum CLUSTERING_MODE {
    NO_CLUSTERING,
    ISOTROPIC_CLUSTERING,
    ORIENTED_CLUSTERING
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

/**
A refinement oracle evaluates if the given candidate
link is admissible for light transport. It returns a refinement action
that will refine the link if necessary. If the link is admissible, it
returns the special action 'dontRefine' which does nothing
*/
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

/**
Global parameters controlling hierarchical refinement
*/
class ElementHierarchyState {
  public:
    float epsilon; // Determines meshing accuracy
    int do_h_meshing; // If doing hierarchical meshing
    float minimumArea; // Minimum allowed element area
    long nr_elements; // Number of elements
    long nr_clusters; // Number of clusters
    int tvertex_elimination; // If doing T-vertex elimination for rendering
    CLUSTERING_MODE clustering; // Clustering mode, 0 => no clustering
    ORACLE oracle; // Refinement oracle to be used
    StochasticRadiosityElement *topCluster; // Top cluster element of element hierarchy
    java::ArrayList<Vector3D *> *coords;
    java::ArrayList<Vector3D *> *normals; // Created during element subdivision
    java::ArrayList<Vector3D *> *texCoords; // Created during element subdivision
    java::ArrayList<Vertex *> *vertices;
};

extern ElementHierarchyState GLOBAL_stochasticRaytracing_hierarchy;

#define DEFAULT_EH_EPSILON 5e-4
#define DEFAULT_EH_MINIMUM_AREA 1e-6
#define DEFAULT_EH_HIERARCHICAL_MESHING true
#define DEFAULT_EH_CLUSTERING ORIENTED_CLUSTERING
#define DEFAULT_EH_T_VERTEX_ELIMINATION true

extern void elementHierarchyDefaults();
extern void elementHierarchyInit(Geometry *clusteredWorldGeometry);
extern void elementHierarchyTerminate(java::ArrayList<Patch *> *scenePatches);

#endif
