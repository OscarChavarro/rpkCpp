#ifndef __RENDER__
#define __RENDER__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/BoundingBox.h"
#include "skin/Geometry.h"
#include "scene/Camera.h"

class Render {
  private:
    static void renderGeomBounds(Camera *camera, const Geometry *geometry);

  public:
    static void renderBoundingBox(BoundingBox boundingBox);
    static void renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries, const RenderOptions *renderOptions);
    static void renderClusterHierarchy(Camera *camera, const java::ArrayList<Geometry *> *clusteredGeometryList, const RenderOptions *renderOptions);
    static void renderGetNearFar(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);
};

#endif
