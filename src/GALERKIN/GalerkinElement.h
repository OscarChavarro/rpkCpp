/**
Galerkin finite elements: one structure for both surface and cluster elements
*/

#ifndef __GALERKIN_ELEMENT__
#define __GALERKIN_ELEMENT__

#include "skin/Patch.h"
#include "GALERKIN/interactionlist.h"
#include "common/linealAlgebra/Matrix2x2.h"
#include "common/linealAlgebra/Matrix4x4.h"
#include "skin/Element.h"
#include "scene/polygon.h"
#include "GALERKIN/elementlistgalerkin.h"

class INTERACTIONLIST;

typedef union FloatOrColorPtr {
    float f;
    COLOR *c;
} FloatOrColorPtr;

typedef union PatchOrGeomPtr {
    Patch *patch;
    Geometry *geom;
} PatchOrGeomPtr;

/**
The Galerkin radiosity specific data to be kept with every surface or
cluster element. A flag indicates whether a given element is a cluster or
surface elements. There are only a few differences between surface and cluster
elements: cluster elements always have a constant basis on them, they contain
a pointer to the Geometry to which they are associated, while a surface element has
a pointer to the Patch to which is belongs, they have only irregular sub-elements,
and no up trans
*/
class GalerkinElement : public Element {
  public:
    COLOR *radiance; // Total radiance on the element as computed so far
    COLOR *receivedRadiance; // Radiance received during iteration
    COLOR *unShotRadiance; // For progressive refinement radiosity
    FloatOrColorPtr potential; // Total potential of the element
    FloatOrColorPtr receivedPotential; // Potential received during the last iteration
    FloatOrColorPtr unShotPotential; // Un-shot potential (progressive refinement radiosity)
    FloatOrColorPtr directPotential;
    INTERACTIONLIST *interactions; /* links with other patches: when using
			 * a shooting algorithm, the links are kept 
			 * with the source element. When doing gathering,
			 * the links are kept with the receiver element. */
    GalerkinElement *parent;    /* parent element in a hierarchy, or
			 * a nullptr pointer if there is no parent. */
    GalerkinElement **regularSubElements; /* A nullptr pointer if there are no
			 * regular sub-elements, or an array containing
			 * exactly 4 pointers to the sub-elements */
    ELEMENTLIST *irregularSubElements; /* nullptr pointer or pointer to
			 * the list of irregular sub-elements */
    Matrix2x2 *uptrans;    /* if non-null, transforms (u,v) coordinates on
			 * a subelement to the (u,v) coordinates of the 
			 * same point on the parent surface element. It is nullptr
			 * if the element is a toplevel element for a patch or
			 * a cluster element. If non-null it is a subelement on
			 * a patch. */
    float area;            /* area of a surface element or sum of the areas
			 * of the surfaces in a cluster element. */
    float minimumArea;    /* equal to area for a surface element or the area
			 * of the smallest surface element in a cluster. */
    float bsize; // Equivalent blocker size for multi-resolution visibility
    int numberOfPatches; // Number of patches in a cluster
    int tmp; // For occasional use
    char childnr; // Rang nr of regular sub-element in parent
    char basisSize; // Number of coefficients to represent radiance
    char basisUsed; // Number of coefficients effectively used (<=basis_size)
    unsigned char flags; // Various flags, see below

    GalerkinElement();
    void printRegularHierarchy(int level);
};

/* element flags: */
#define INTERACTIONS_CREATED    0x01    /* set when all interactions have
					 * been created for a toplevel element. */
#define IS_CLUSTER        0x10    /* if set, indicates that the element is
					 * a cluster element. If not set, the element is
					 * a surface element. */
#define IS_LIGHT_SOURCE        0x20    /* whether or not the element is
					 * or contains surfaces emitting
					 * light spontaneously. */

inline bool
isCluster(GalerkinElement *element) {
    return element->flags & IS_CLUSTER;
}

#define IsLightSource(element)    (element->flags & IS_LIGHT_SOURCE)

/**
Calls 'routine' for every regular sub-element of the element (if there
are any). 'routine' should have one argument: an ELEMENT *
*/
#define ITERATE_REGULAR_SUB_ELEMENTS(element, routine) \
    if ( (element)->regularSubElements != nullptr ) { \
        for ( int i = 0; i < 4; i++) { \
            (routine)((element)->regularSubElements[i]); \
        } \
    }

/**
Same, but for the irregular sub-elements
*/
#define ITERATE_IRREGULAR_SUBELEMENTS(element, routine) \
  ElementListIterate(element->irregularSubElements, routine)

#define ForAllRegularSubelements(child, elem) { \
  if ((elem)->regularSubElements) { \
    int i; \
    for ( i = 0; i < 4; i++) { \
      GalerkinElement *child = (elem)->regularSubElements[i];

#define ForAllIrregularSubelements(child, elem) { \
  if ((elem)->irregularSubElements) { \
    ELEMENTLIST *ellist; \
    for (ellist = (elem)->irregularSubElements; ellist; ellist=ellist->next) { \
      GalerkinElement *child = ellist->element;

/**
Position and orientation of the regular sub-elements is fully
determined by the following transforms, that transform (u,v)
parameters of a point on a sub-element to the (u',v') parameters
of the same point on the parent element
*/
extern Matrix2x2 GLOBAL_galerkin_QuadUpTransformMatrix[4];
extern Matrix2x2 GLOBAL_galerkin_TriangularUpTransformMatrix[4];

extern int galerkinElementGetNumberOfElements();
extern int galerkinElementGetNumberOfClusters();
extern int galerkinElementGetNumberOfSurfaceElements();
extern GalerkinElement *galerkinElementCreateTopLevel(Patch *patch);
extern GalerkinElement *galerkinElementCreateCluster(Geometry *geometry);
extern GalerkinElement **galerkinElementRegularSubDivide(GalerkinElement *element);
extern void galerkinElementPrint(FILE *out, GalerkinElement *element);
extern void galerkinElementPrintId(FILE *out, GalerkinElement *elem);
extern void galerkinElementDestroyTopLevel(GalerkinElement *element);
extern void galerkinElementDestroyCluster(GalerkinElement *element);
extern Matrix2x2 *galerkinElementToTopTransform(GalerkinElement *element, Matrix2x2 *xf);
extern GalerkinElement *galerkinElementRegularSubElementAtPoint(GalerkinElement *parent, double *u, double *v);
extern GalerkinElement *galerkinElementRegularLeafAtPoint(GalerkinElement *top, double *u, double *v);
extern void galerkinElementDrawOutline(GalerkinElement *elem);
extern void galerkinElementRender(GalerkinElement *elem);
extern void galerkinElementReAllocCoefficients(GalerkinElement *element);
extern int galerkinElementVertices(GalerkinElement *elem, Vector3D *p);
extern float *galerkinElementBounds(GalerkinElement *elem, float *bounds);
extern Vector3D galerkinElementMidPoint(GalerkinElement *elem);
extern POLYGON *galerkinElementPolygon(GalerkinElement *elem, POLYGON *poly);
extern void forAllLeafElements(GalerkinElement *top, void (*func)(GalerkinElement *));

#endif
