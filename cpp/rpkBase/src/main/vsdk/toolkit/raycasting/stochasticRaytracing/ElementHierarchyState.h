#ifndef __ELEMENT_HIERARCHY_STATE__
#define __ELEMENT_HIERARCHY_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Link.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/scene/RadianceElementTable.h"

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
    java::ArrayList<Vector3Df *> *coords;
    java::ArrayList<Vector3Df *> *normals; // Created during element subdivision
    java::ArrayList<Vector3Df *> *texCoords; // Created during element subdivision
    java::ArrayList<RpkVertex *> *vertices;

    ElementHierarchyState();
    static void setActiveState(ElementHierarchyState &state);
    static ElementHierarchyState &activeState();

    RadianceElementTable *getRadianceElements() const;
    void setRadianceElements(RadianceElementTable *inRadianceElements);

  private:
    // The scene's table of top-level elements, bound when the method is initialized;
    // null while the method has not been initialized (nothing to terminate then)
    RadianceElementTable *radianceElements;
    static ElementHierarchyState *&activeStatePtr();
};

inline RadianceElementTable *
ElementHierarchyState::getRadianceElements() const {
    return radianceElements;
}

inline void
ElementHierarchyState::setRadianceElements(RadianceElementTable *inRadianceElements) {
    radianceElements = inRadianceElements;
}

#endif
