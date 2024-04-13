#ifndef __RENDER__
#define __RENDER__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/BoundingBox.h"
#include "skin/Geometry.h"

extern void renderBounds(BoundingBox bounds);
extern void renderBoundingBoxHierarchy();
extern void renderClusterHierarchy(java::ArrayList<Geometry *> *clusteredGeometryList);
extern void renderSetBackfaceCulling(char truefalse);
extern void renderSetSmoothShading(char truefalse);
extern void renderSetOutlineDrawing(char truefalse);
extern void renderSetBoundingBoxDrawing(char truefalse);
extern void renderSetClusterDrawing(char truefalse);
extern void renderUseDisplayLists(char truefalse);
extern void renderUseFrustumCulling(char truefalse);
extern void renderSetNoShading(char truefalse);
extern void renderSetOutlineColor(ColorRgb *outline_color);
extern void renderSetBoundingBoxColor(ColorRgb *outline_color);
extern void renderSetClusterColor(ColorRgb *cluster_color);
extern void renderGetNearFar(float *near, float *far, java::ArrayList<Geometry *> *sceneGeometries);
extern void renderParseOptions(int *argc, char **argv);

#endif
