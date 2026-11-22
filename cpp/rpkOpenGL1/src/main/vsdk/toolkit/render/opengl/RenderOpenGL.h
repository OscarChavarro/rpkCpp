#ifndef RENDER_OPEN_GL__
#define RENDER_OPEN_GL__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/scene/Camera.h"

class RenderOpenGL {
  private:
    static void renderGeomBounds(Camera *camera, const RpkGeometry *geometry);

  public:
    static void renderBoundingBox(RpkAxisAlignedBoundingBox boundingBox);
    static void renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries, const RendererConfiguration *renderOptions);
    static void renderClusterHierarchy(Camera *camera, const java::ArrayList<RpkGeometry *> *clusteredGeometryList, const RendererConfiguration *renderOptions);
    static void renderGetNearFar(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries);
};

#endif
