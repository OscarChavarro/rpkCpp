#ifndef __ELEMENT__
#define __ELEMENT__

#include "common/RenderOptions.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Matrix2x2.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"
#include "skin/ElementFlags.h"
#include "skin/ElementTypes.h"

class Element {
  public:
    int id; // Unique ID number for the element
    ColorRgb Ed; // Diffuse emittance radiance
    ColorRgb Rd; // Reflectance
    ColorRgb *radiance; // Total radiance on the element as computed so far
    ColorRgb *receivedRadiance; // Radiance received during iteration
    ColorRgb *unShotRadiance; // For progressive refinement radiosity
    float area; // Area of all surfaces contained in the element
    ElementTypes className;
    unsigned char flags;

    Patch *patch;
    Geometry *geometry;

    Element *parent; // Parent element in a hierarchy, or nullptr pointer if there is no parent
    Element **regularSubElements; // For surface elements with regular quadtree subdivision
        // A nullptr pointer if there are no regular sub-elements (child element in hierarchy), or a 4-sized
        // array containing with sub-elements. Note both triangles and quads are subdivided in 4.
    java::ArrayList<Element *> *irregularSubElements; // Hierarchy of clusters
    Matrix2x2 *transformToParent; // Relates surface element (u, v) coordinates to patch (u, v) coordinates,
        // if non-null, transforms (u, v) coordinates on a sub-element to the (u, v) coordinates
        // of the same point on the parent surface element. It is nullptr if the element is a
        // toplevel element for a patch or a cluster element. If non-null it is a sub-element on a patch

    Element();
    virtual ~Element() {};

    inline bool
    isCluster() const {
        return flags & IS_CLUSTER_MASK;
    }

    Matrix2x2 *topTransform(Matrix2x2 *xf) const;
    void traverseAllLeafElements(void (*traversalCallbackFunction)(Element *));
    void traverseClusterLeafElements(void (*traversalCallbackFunction)(Element *));
    void traverseQuadTreeLeafs(void (*traversalCallbackFunction)(Element *, const RenderOptions *renderOptions), const RenderOptions *renderOptions);

#ifdef RAYTRACING_ENABLED
    bool isLeaf() const;
    Element *childContainingElement(Element *descendant);
    bool traverseAllChildren(void (*traversalCallbackFunction)(Element *)) const;
#endif
};

#endif
