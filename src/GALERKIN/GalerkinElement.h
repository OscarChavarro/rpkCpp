/* element.h: Galerkin finite elements: one structure for both surface and cluster
1;95;0c * elements.  */

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

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

/* the Galerkin radiosity specific data to be kept with every surface or 
 * cluster element. A flag indicates whether a given element is a cluster or
 * surface elements. There are only a few differences between surface and cluster 
 * elements: cluster elements always have a constant basis on them, they contain
 * a pointer to the Geometry to which they are associated, while a surface element has
 * a pointer to the Patch to which is belongs, they have only irregular subelements,
 * and no uptrans. */
class GalerkinElement : public Element {
  public:
    COLOR *radiance,    /* total radiance on the element as computed so far */
    *received_radiance, /* radiance received during iteration */
    *unshot_radiance; /* for progressive refinement radiosity */
    FloatOrColorPtr potential, received_potential, unshot_potential, direct_potential;
    /* total potential of the element, potential
* received during the last iteration and unshot
* potential (progressive refinement radiosity). */
    INTERACTIONLIST *interactions; /* links with other patches: when using
			 * a shooting algorithm, the links are kept 
			 * with the source element. When doing gathering,
			 * the links are kept with the receiver element. */
    GalerkinElement *parent;    /* parent element in a hierarchy, or
			 * a nullptr pointer if there is no parent. */
    GalerkinElement **regular_subelements;    /* a nullptr pointer if there are no
			 * regular sub-elements, or an array containing
			 * exactly 4 pointers to the sub-elements. */
    ELEMENTLIST *irregular_subelements;    /* nullptr pointer or pointer to
			 * the list of irregular subelements. */
    Matrix2x2 *uptrans;    /* if non-null, transforms (u,v) coordinates on
			 * a subelement to the (u,v) coordinates of the 
			 * same point on the parent surface element. It is nullptr
			 * if the element is a toplevel element for a patch or
			 * a cluster element. If non-null it is a subelement on
			 * a patch. */
    float area;            /* area of a surface element or sum of the areas
			 * of the surfaces in a cluster element. */
    float minarea;    /* equal to area for a surface element or the area
			 * of the smallest surface element in a cluster. */
    float bsize;        /* Equivalent blocker size for multiresoltuin visibility */
    int nrpatches;    /* number of patches in a cluster */
    int tmp;        /* for occasional use */
    char childnr;        /* rang nr of regular subelement in parent */
    char basis_size;    /* nr of coefficients to represent radiance ... */
    char basis_used;    /* nr of coefficients effectively used (<=basis_size) */
    unsigned char flags;    /* various flags, see below */
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

/* calls 'routine' for every regular subelement of the element (if there
 * are any). 'routine' should have one argument: an ELEMENT *. */
#define ITERATE_REGULAR_SUBELEMENTS(element, routine)        \
  if ((element)->regular_subelements) {                \
    int i;                            \
    for (i=0; i<4; i++)                        \
      (routine)((element)->regular_subelements[i]);        \
  }

/* same, but for the irregular subelements. */
#define ITERATE_IRREGULAR_SUBELEMENTS(element, routine)    \
  ElementListIterate(element->irregular_subelements, routine)

#define ForAllRegularSubelements(child, elem) {        \
  if ((elem)->regular_subelements) {                \
    int i;                            \
    for (i=0; i<4; i++) {                    \
      GalerkinElement *child = (elem)->regular_subelements[i] ;        \

#define ForAllIrregularSubelements(child, elem) {        \
  if ((elem)->irregular_subelements) {                \
    ELEMENTLIST *ellist;                    \
    for (ellist = (elem)->irregular_subelements; ellist; ellist=ellist->next) {\
      GalerkinElement *child = ellist->element;            \

/**
Position and orientation of the regular subelements is fully
determined by the following transforms, that transform (u,v)
parameters of a point on a subelement to the (u',v') parameters
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
