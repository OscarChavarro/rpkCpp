#ifndef __ELEMENT_HIERARCHY_STATE__
#define __ELEMENT_HIERARCHY_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/HierarchyClusteringMode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Link.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/scene/RadianceElementTable.h"

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
    double epsilon; // Determines meshing accuracy
    int do_h_meshing; // If doing hierarchical meshing
    double minimumArea; // Minimum allowed element area
    long nr_elements; // Number of elements
    long nr_clusters; // Number of clusters
    int tvertex_elimination; // If doing T-vertex elimination for rendering
    HierarchyClusteringMode clustering; // Clustering mode, 0 => no clustering
    ORACLE oracle; // Refinement oracle to be used
    StochasticRadiosityElement *topCluster; // Top cluster element of element hierarchy
    ArrayList<Vector3Dd *> *coords;
    ArrayList<Vector3Dd *> *normals; // Created during element subdivision
    ArrayList<Vector3Dd *> *texCoords; // Created during element subdivision
    ArrayList<RpkVertex *> *vertices;

    ElementHierarchyState();
    static void setActiveState(ElementHierarchyState &state);
    static ElementHierarchyState &activeState();

    RadianceElementTable *getRadianceElements() const;
    void setRadianceElements(RadianceElementTable *inRadianceElements);

  private:
    static ElementHierarchyState *&activeStatePtr();
    // The scene's table of top-level elements, bound when the method is initialized;
    // null while the method has not been initialized (nothing to terminate then)
    RadianceElementTable *radianceElements;
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
