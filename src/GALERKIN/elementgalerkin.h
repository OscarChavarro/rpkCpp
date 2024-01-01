/* element.h: Galerkin finite elements: one structure for both surface and cluster
1;95;0c * elements.  */

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include "skin/patch.h"
#include "GALERKIN/interactionlist.h"
#include "common/linealAlgebra/Matrix2x2.h"
#include "common/linealAlgebra/Matrix4x4.h"
#include "scene/polygon.h"
#include "GALERKIN/elementlistgalerkin.h"

typedef union FloatOrColorPtr {
    float f;
    COLOR *c;
} FloatOrColorPtr;

typedef union PatchOrGeomPtr {
    PATCH *patch;
    Geometry *geom;
} PatchOrGeomPtr;

/* the Galerkin radiosity specific data to be kept with every surface or 
 * cluster element. A flag indicates whether a given element is a cluster or
 * surface elements. There are only a few differences between surface and cluster 
 * elements: cluster elements always have a constant basis on them, they contain
 * a pointer to the Geometry to which they are associated, while a surface element has
 * a pointer to the PATCH to which is belongs, they have only irregular subelements,
 * and no uptrans. */
class ELEMENT {
  public:
    int id;        /* unique ID number for the element */
    PatchOrGeomPtr pog;    /* The PATCH/Geometry to which the surface/cluster element
			 * belongs. */
    COLOR Ed, Rd;        /* diffuse emittance and reflectance */
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
    ELEMENT *parent;    /* parent element in a hierarchy, or
			 * a nullptr pointer if there is no parent. */
    ELEMENT **regular_subelements;    /* a nullptr pointer if there are no
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

#define IsCluster(element)    (element->flags & IS_CLUSTER)
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
      ELEMENT *child = (elem)->regular_subelements[i] ;        \

#define ForAllIrregularSubelements(child, elem) {        \
  if ((elem)->irregular_subelements) {                \
    ELEMENTLIST *ellist;                    \
    for (ellist = (elem)->irregular_subelements; ellist; ellist=ellist->next) {\
      ELEMENT *child = ellist->element;            \


/* returns the total number of elements in use */
extern int GetNumberOfElements();

extern int GetNumberOfClusters();

extern int GetNumberOfSurfaceElements();

/* creates the toplevel element for the patch */
extern ELEMENT *CreateToplevelElement(PATCH *patch);

/* creates a cluster element for the given Geometry. The average projected area still
 * needs to be determined. */
extern ELEMENT *CreateClusterElement(Geometry *geom);

/* Regularly subdivides the given element. A pointer to an array of
 * 4 pointers to subelements is returned. Only for surface elements. */
extern ELEMENT **RegularSubdivideElement(ELEMENT *element);

/* position and orientation of the regular subelements is fully
 * determined by the following transforms, that transform (u,v)
 * parameters of a point on a subelement to the (u',v') parameters
 * of the same point on the parent element. */
extern Matrix2x2 quadupxfm[4], triupxfm[4];

/* prints the element data to the file 'out' */
extern void PrintElement(FILE *out, ELEMENT *element);

/* prints element patch id and the chain of child numbers of an element
 * in a hierarchy */
extern void PrintElementId(FILE *out, ELEMENT *elem);

/* destroys the toplevel surface element and it's subelements (recursive) */
extern void DestroyToplevelElement(ELEMENT *element);

/* destroys the cluster element, not recursive. */
extern void DestroyClusterElement(ELEMENT *element);

/* Computes the transform relating a surface element to the toplevel element in the
 * hierarchy by concatenaing the up-transforms of the element and all parent 	
 * alements. If the element is a toplevel element, (Matrix4x4 *)nullptr is
 * returned and nothing is filled in in xf (no trnasform is necessary
 * to transform points on the element to the corresponding point on the toplevel
 * element). In the other case, the composed transform is filled in in xf and
 * xf (pointer to the transform) is returned. */
extern Matrix2x2 *ElementToTopTransform(ELEMENT *element, Matrix2x2 *xf);

/* Determines the regular subelement at point (u,v) of the given parent
 * surface element. Returns the parent element itself if there are no regular 
 * subelements. The point is transformed to the corresponding point on the subelement. */
extern ELEMENT *RegularSubelementAtPoint(ELEMENT *parent, double *u, double *v);

/* Returns the leaf regular subelement of 'top' at the point (u,v) (uniform 
 * coordinates!). (u,v) is transformed to the coordinates of the corresponding
 * point on the leaf element. 'top' is a surface element. */
extern ELEMENT *RegularLeafElementAtPoint(ELEMENT *top, double *u, double *v);

/* draws element outline in the current outline color */
extern void DrawElementOutline(ELEMENT *elem);

/* renders a surface element flat shaded based on its radiance. */
extern void RenderElement(ELEMENT *elem);

/* (re)allocates storage for the coefficients to represent radiance, received radiance
 * and unshot radiance on the element. */
extern void ElementReallocCoefficients(ELEMENT *elem);

/* Computes the vertices of a surface element (3 or 4 vertices) or
 * cluster element (8 vertices). The number of vertices is returned. */
extern int ElementVertices(ELEMENT *elem, Vector3D *p);

/* Computes a bounding box for the element. */
extern float *ElementBounds(ELEMENT *elem, float *bounds);

/* Computes the midpoint of the element. */
extern Vector3D ElementMidpoint(ELEMENT *elem);

/* Computes a polygon description for shaft culling for the element. */
extern POLYGON *ElementPolygon(ELEMENT *elem, POLYGON *poly);

/* Call func for each leaf element of top */
extern void ForAllLeafElements(ELEMENT *top, void (*func)(ELEMENT *));

#endif
