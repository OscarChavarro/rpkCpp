/**
Hierarchical refinement stuff (includes Jan's elementP.h)
*/

#ifndef __ELEMENT_HIERARCHY__
#define __ELEMENT_HIERARCHY__

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

/* a link contains a pointer to the receiver and source elements */
class LINK {
  public:
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
};

/**
A refinement action takes a LINK, performs some refinement action and
returns a pointer to the refined link. The parameters (us,vs) and
(ur,vr) are transformed to become the parameters of the same point on the
sub-element resulting after refinement
*/
typedef LINK *(*REFINE_ACTION)(
    LINK *link,
    StochasticRadiosityElement *rcvtop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srctop,
    double *us,
    double *vs,
    const RenderOptions *renderOptions);

/**
A refinement oracle evaluates if the given candidate
link is admissible for light transport. It returns a refinement action
that will refine the link if necessary. If the link is admissible, it
returns the special action 'dontRefineCallBack' which does nothing
*/
typedef REFINE_ACTION (*ORACLE)(const LINK *link);

extern REFINE_ACTION powerOracle(const LINK *link);
extern LINK topLink(StochasticRadiosityElement *rcvTop, StochasticRadiosityElement *srcTop);
extern LINK *hierarchyRefine(
    LINK *link,
    StochasticRadiosityElement *rcvTop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srcTop,
    double *us,
    double *vs,
    ORACLE evaluateLink,
    const RenderOptions *renderOptions);

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
    HierarchyClusteringMode clustering; // Clustering mode, 0 => no clustering
    ORACLE oracle; // Refinement oracle to be used
    StochasticRadiosityElement *topCluster; // Top cluster element of element hierarchy
    java::ArrayList<Vector3D *> *coords;
    java::ArrayList<Vector3D *> *normals; // Created during element subdivision
    java::ArrayList<Vector3D *> *texCoords; // Created during element subdivision
    java::ArrayList<Vertex *> *vertices;
};

extern ElementHierarchyState GLOBAL_stochasticRaytracing_hierarchy;

extern void elementHierarchyDefaults();
extern void elementHierarchyInit(Geometry *clusteredWorldGeometry);
extern void elementHierarchyTerminate(const java::ArrayList<Patch *> *scenePatches);

#endif
