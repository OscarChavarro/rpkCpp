#ifndef __RENDER__
#define __RENDER__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/BoundingBox.h"
#include "skin/Geometry.h"
#include "scene/Camera.h"

extern void renderBoundingBox(BoundingBox boundingBox);
extern void renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries, const RenderOptions *renderOptions);
extern void renderClusterHierarchy(Camera *camera, const java::ArrayList<Geometry *> *clusteredGeometryList, const RenderOptions *renderOptions);
extern void renderGetNearFar(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);
extern void renderParseOptions(int *argc, char **argv, RenderOptions *renderOptions);

#endif
