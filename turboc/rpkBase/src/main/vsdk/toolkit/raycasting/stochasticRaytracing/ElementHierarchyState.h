#ifndef __ELEMENT_HIERARCHY_STATE__
#define __ELEMENT_HIERARCHY_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Link.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

typedef Link *(*REFINE_ACTION)(
    Link *link,
    StochasticRadiosityElement *rcvtop,
    double *ur,
    double *vr,
    StochasticRadiosityElement *srctop,
    double *us,
    double *vs,
    const RenderOptions *renderOptions);

typedef REFINE_ACTION (*ORACLE)(const Link *link);

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
    ArrayList<Vector3Df *> *coords;
    ArrayList<Vector3Df *> *normals; // Created during element subdivision
    ArrayList<Vector3Df *> *texCoords; // Created during element subdivision
    ArrayList<Vertex *> *vertices;

    ElementHierarchyState();
    static void setActiveState(ElementHierarchyState &state);
    static ElementHierarchyState &activeState();

  private:
    static ElementHierarchyState *&activeStatePtr();
};

#endif
