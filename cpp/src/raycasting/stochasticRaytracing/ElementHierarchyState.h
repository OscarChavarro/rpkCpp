#ifndef __ELEMENT_HIERARCHY_STATE__
#define __ELEMENT_HIERARCHY_STATE__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "material/RendererConfiguration.h"
#include "environment/geometry/elements/Vertex.h"
#include "raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "raycasting/stochasticRaytracing/Link.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

using REFINE_ACTION = Link *(*)(
    Link *link,
    StochasticRadiosityElement *rcvtop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srctop,
    double *us,
    double *vs,
    const RendererConfiguration *renderOptions);

using ORACLE = REFINE_ACTION (*)(const Link *link);

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

    ElementHierarchyState();
    static void setActiveState(ElementHierarchyState &state);
    static ElementHierarchyState &activeState();

  private:
    static ElementHierarchyState *&activeStatePtr();
};

#endif
