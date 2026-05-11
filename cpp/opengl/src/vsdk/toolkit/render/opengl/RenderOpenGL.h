#ifndef RENDER_OPEN_GL__
#define RENDER_OPEN_GL__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/skin/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/scene/Camera.h"

class RenderOpenGL {
  private:
    static void renderGeomBounds(Camera *camera, const Geometry *geometry);

  public:
    static void renderBoundingBox(AxisAlignedBoundingBox boundingBox);
    static void renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries, const RendererConfiguration *renderOptions);
    static void renderClusterHierarchy(Camera *camera, const java::ArrayList<Geometry *> *clusteredGeometryList, const RendererConfiguration *renderOptions);
    static void renderGetNearFar(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);
};

#endif
