#ifndef __RENDER__
#define __RENDER__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/bounds.h"

extern void renderBounds(BOUNDINGBOX bounds);
extern void renderBoundingBoxHierarchy();
extern void renderClusterHierarchy();
extern void renderSetBackfaceCulling(char truefalse);
extern void renderSetSmoothShading(char truefalse);
extern void renderSetOutlineDrawing(char truefalse);
extern void renderSetBoundingBoxDrawing(char truefalse);
extern void renderSetClusterDrawing(char truefalse);
extern void renderUseDisplayLists(char truefalse);
extern void renderUseFrustumCulling(char truefalse);
extern void renderSetNoShading(char truefalse);
extern void renderSetOutlineColor(RGB *outline_color);
extern void renderSetBoundingBoxColor(RGB *outline_color);
extern void renderSetClusterColor(RGB *cluster_color);
extern void renderGetNearFar(float *near, float *far);
extern void renderParseOptions(int *argc, char **argv);

#endif
