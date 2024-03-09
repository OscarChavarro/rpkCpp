#ifndef __ELEMENT__
#define __ELEMENT__

#include "common/linealAlgebra/Matrix2x2.h"
#include "common/color.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"

enum ElementTypes {
    ELEMENT_GALERKIN,
    ELEMENT_STOCHASTIC_RADIOSITY
};

// If set, indicates that the element is a cluster element. If not set, the element is a surface element
#define IS_CLUSTER_MASK 0x01

// If the element is or contains surfaces emitting light spontaneously
#define IS_LIGHT_SOURCE_MASK 0x02

// Set when all interactions have been created for a toplevel element
#define INTERACTIONS_CREATED_MASK 0x04

class Element {
  public:
    int id; // Unique ID number for the element
    COLOR Ed; // Diffuse emittance radiance
    COLOR Rd; // Reflectance
    Patch *patch;
    Geometry *geometry;
    COLOR *radiance; // Total radiance on the element as computed so far
    COLOR *receivedRadiance; // Radiance received during iteration
    COLOR *unShotRadiance; // For progressive refinement radiosity
    Element *parent; // Parent element in a hierarchy, or
        // nullptr pointer if there is no parent
    Element **regularSubElements; // For surface elements with regular quadtree subdivision
        // A nullptr pointer if there are no regular sub-elements, or an array containing
        // exactly 4 pointers to the sub-elements
    java::ArrayList<Element *> *irregularSubElements; // Hierarchy of clusters
    Matrix2x2 *upTrans; // Relates surface element (u,v) coordinates to patch (u,v) coordinates,
    // if non-null, transforms (u,v) coordinates on a sub-element to the (u,v) coordinates
    // of the same point on the parent surface element. It is nullptr if the element is a
    // toplevel element for a patch or a cluster element. If non-null it is a sub-element on a patch
    float area; // Area of all surfaces contained in the element
    ElementTypes className;
    unsigned char flags;

    Element();
    virtual ~Element() {};

    inline bool
    isCluster() const {
        return flags & IS_CLUSTER_MASK;
    }

    Matrix2x2 *topTransform(Matrix2x2 *xf);
    bool isLeaf() const;
    Element *childContainingElement(Element *descendant);
    void traverseAllLeafElements(void (*traversalCallbackFunction)(Element *));
    void traverseClusterLeafElements(void (*traversalCallbackFunction)(Element *));
};

#endif
