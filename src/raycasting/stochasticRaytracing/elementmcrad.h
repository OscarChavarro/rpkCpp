/* element.h */

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

/* Data associated with each PATCH: */
#include "raycasting/stochasticRaytracing/elementtype.h"

/* close these macros with EndForAll */
#define ForAllRegularSubelements(p, parent) {    \
if ((parent)->regular_subelements) {        \
  int i;                    \
  for (i=0; i<4; i++) {                \
    ELEMENT *p = (parent)->regular_subelements[i];

#define ForAllIrregularSubelements(p, parent) { \
if ((parent)->irregular_subelements) {        \
  ELEMENTLIST *_el_;            \
  for (_el_ = (parent)->irregular_subelements; _el_; _el_ = _el_->next) { \
    ELEMENT *p = _el_->element;

#define ForAllVerticesOfElement(vert, elem) {    \
  int _i_;                    \
  for (_i_=0; _i_<(elem)->nrvertices; _i_++) {    \
    VERTEX *(vert) = (elem)->vertex[_i_]; {

extern ELEMENT *CreateToplevelSurfaceElement(PATCH *patch);

extern void McrDestroyToplevelSurfaceElement(ELEMENT *elem);

extern ELEMENT *McrCreateClusterHierarchy(Geometry *world);

extern void McrDestroyClusterHierarchy(ELEMENT *top);

extern void McrPrintElement(FILE *out, ELEMENT *elem);

extern void McrForAllLeafElements(ELEMENT *top, void (*func)(ELEMENT *));

extern void McrForAllSurfaceLeafs(ELEMENT *top, void (*func)(ELEMENT *));

/* returns true if top has children and returns false if top is a leaf element */
extern int McrForAllChildrenElements(ELEMENT *top, void (*func)(ELEMENT *));

/* returns true if elem is a leaf element */
extern int ElementIsLeaf(ELEMENT *elem);

extern void ElementRange(ELEMENT *elem, int *nbits, niedindex *msb1, niedindex *rmsb2);

extern float *McrElementBounds(ELEMENT *elem, float *bounds);

extern ELEMENT *ClusterChildContainingElement(ELEMENT *parent, ELEMENT *descendant);

extern ELEMENT **McrRegularSubdivideElement(ELEMENT *element);

extern ELEMENT *monteCarloRadiosityRegularSubElementAtPoint(ELEMENT *parent, double *u, double *v);

extern ELEMENT *McrRegularLeafElementAtPoint(ELEMENT *top, double *u, double *v);

extern VERTEX *McrEdgeMidpointVertex(ELEMENT *elem, int edgenr);

extern Matrix2x2 mcr_quadupxfm[4];
extern Matrix2x2 mcr_triupxfm[4];

/* only for surface elements!! */
extern int ElementIsTextured(ELEMENT *elem);

/* uses elem->Rd for surface elements */
extern float ElementScalarReflectance(ELEMENT *elem);

/* implemented pushpull.c and basis.c */
extern void PushRadiance(ELEMENT *parent, ELEMENT *child, COLOR *parent_rad, COLOR *child_rad);

extern void PushImportance(ELEMENT *parent, ELEMENT *child, float *parent_imp, float *child_imp);

extern void PullRadiance(ELEMENT *parent, ELEMENT *child, COLOR *parent_rad, COLOR *child_rad);

extern void PullImportance(ELEMENT *parent, ELEMENT *child, float *parent_imp, float *child_imp);

/* implemented in render.c */
extern COLOR ElementDisplayRadiance(ELEMENT *elem);

extern COLOR ElementDisplayRadianceAtPoint(ELEMENT *elem, double u, double v);

extern void McrRenderElement(ELEMENT *elem);

extern void RenderElementOutline(ELEMENT *elem);

extern void ElementComputeNewVertexColors(ELEMENT *elem);

extern void ElementAdjustTVertexColors(ELEMENT *elem);

extern RGB ElementColor(ELEMENT *elem);

extern COLOR VertexReflectance(VERTEX *v);

extern void ElementTVertexElimination(ELEMENT *elem,
                                      void (*do_triangle)(VERTEX *, VERTEX *, VERTEX *),
                                      void (*do_quadrilateral)(VERTEX *, VERTEX *, VERTEX *, VERTEX *));

/* don't overuse the macros below. There are often elegant alternatives. */

/* maximum element hierarchy depth: Fatal errors in the macros below if
 * the element hierarchy is deeper than this, but that is surely not a
 * normal situation anyways! */
#define MAX_HIERARCHY_DEPTH 100

#include "common/dataStructures/stackmac.h"

/* iterates over all toplevel surface element in the element hierarchy
 * with the cluster element 'top' on top.
 *
 * Usage:
 * 
 *   ELEMENT *cluster = (some cluster element)
 *   REC_ForAllClusterSurfaces(surf, cluster) {
 *      do something with ELEMENT 'surf' 
 *   } REC_EndForAllClusterSurfaces;    <---- don't forget!!!
 */
#define REC_ForAllClusterSurfaces(surface, top) {            \
  int _did_recurse = false;                        \
  ELEMENTLIST *_subelp = (ELEMENTLIST*)nullptr;            \
  STACK_DECL(ELEMENTLIST*, _selstack, MAX_HIERARCHY_DEPTH, _selp);    \
  ELEMENT *_curel = (top);                        \
  _begin_recurse_CS:                            \
  if (_curel->iscluster) {                        \
    _did_recurse = true;                        \
    STACK_SAVE(_subelp, _selstack, MAX_HIERARCHY_DEPTH, _selp);        \
    _subelp = _curel->irregular_subelements;                \
    while (_subelp) {                            \
      _curel = _subelp->element;                    \
      goto _begin_recurse_CS;                        \
    _end_recurse_CS:                            \
      _subelp = _subelp->next;                        \
    }                                    \
    STACK_RESTORE_NOCHECK(_subelp, _selstack, _selp);            \
    if (_subelp)    /* not back at top */                \
      goto _end_recurse_CS;                        \
  } else {                                \
    ELEMENT *surface = _curel;

/* do something with 'surface' */

#define REC_EndForAllClusterSurfaces                    \
    if (_did_recurse)                            \
      goto _end_recurse_CS;                        \
  }                                    \
}

/* iterates over all leaf elements in the surface element hierarchy with
 * 'top' (a surface element) on top */
#define REC_ForAllSurfaceLeafs(leaf, top) {                \
  int _i_ = -1, _did_recurse = false;                    \
  STACK_DECL(int, _isave, MAX_HIERARCHY_DEPTH, _isaveptr);        \
  ELEMENT *_curel = (top);                        \
  _begin_recurse_SL:                            \
  if (_curel->regular_subelements) { /* not a leaf */            \
    _did_recurse = true;                        \
    STACK_SAVE(_i_, _isave, MAX_HIERARCHY_DEPTH, _isaveptr);        \
    for (_i_=0; _i_<4; _i_++) {                        \
      _curel = _curel->regular_subelements[_i_];            \
      goto _begin_recurse_SL;                        \
    _end_recurse_SL:                            \
      _curel = _curel->parent;                        \
    }                                    \
    STACK_RESTORE_NOCHECK(_i_, _isave, _isaveptr);            \
    if (_curel != (top))    /* not back at top */            \
      goto _end_recurse_SL;                        \
  } else {                                \
    ELEMENT *leaf = _curel;

/* do something with 'leaf' */

#define REC_EndForAllSurfaceLeafs                    \
    if (_did_recurse)                            \
      goto _end_recurse_SL;                        \
  }                                    \
}

#define ForAllElementsSharingVertex(elem, v) ForAllElements(elem, (v)->radiance_data)

#endif
