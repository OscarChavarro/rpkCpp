#ifndef ELEMENT__
#define ELEMENT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix2x2.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/ElementFlags.h"
#include "vsdk/toolkit/environment/geometry/elements/ElementTypes.h"

class Element {
  public:
    int id; // Unique ID number for the element
    ColorRgbMutable Ed; // Diffuse emittance radiance
    ColorRgbMutable Rd; // Reflectance
    ColorRgbMutable *radiance; // Total radiance on the element as computed so far
    ColorRgbMutable *receivedRadiance; // Radiance received during iteration
    ColorRgbMutable *unShotRadiance; // For progressive refinement radiosity
    float area; // Area of all surfaces contained in the element
    ElementTypes className;
    unsigned char flags;

    Element *parent; // Parent element in a hierarchy, or nullptr pointer if there is no parent
    Element **regularSubElements; // For surface elements with regular quadtree subdivision
        // A nullptr pointer if there are no regular sub-elements (child element in hierarchy), or a 4-sized
        // array containing with sub-elements. Note both triangles and quads are subdivided in 4.
    java::ArrayList<Element *> *irregularSubElements; // Hierarchy of clusters
    const Matrix2x2 *transformToParent; // Relates surface element (u, v) coordinates to patch (u, v) coordinates,
        // if non-null, transforms (u, v) coordinates on a sub-element to the (u, v) coordinates
        // of the same point on the parent surface element. It is nullptr if the element is a
        // toplevel element for a patch or a cluster element. If non-null it is a sub-element on a patch

    Element();
    virtual ~Element();

    bool isCluster() const;
    Matrix2x2 *topTransform(Matrix2x2 *transform) const;
    void traverseAllLeafElements(void (*traversalCallbackFunction)(Element *));
    void traverseClusterLeafElements(void (*traversalCallbackFunction)(Element *));
    void traverseQuadTreeLeafs(void (*traversalCallbackFunction)(Element *, const RendererConfiguration *renderOptions), const RendererConfiguration *renderOptions);

#ifdef RAYTRACING_ENABLED
    bool isLeaf() const;
    Element *childContainingElement(Element *descendant);
    bool traverseAllChildren(void (*traversalCallbackFunction)(Element *)) const;
#endif
};

inline
Element::~Element() {
}

inline bool
Element::isCluster() const {
    return flags & IS_CLUSTER_MASK;
}

#endif
