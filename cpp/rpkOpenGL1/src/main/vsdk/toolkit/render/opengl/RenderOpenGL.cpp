/**
Rendering stuff independent of the graphics library being used
*/

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/opengl/Opengl.h"
#include "vsdk/toolkit/render/opengl/RenderOpenGL.h"

/**
Computes the camera near and far clipping distances from the bounding box of
the provided scene geometries, projected onto the camera Z axis.
Adds a small safety margin and fallback defaults for empty or non-positive ranges.
*/
void
RenderOpenGL::renderGetNearFar(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries) {
    RpkAxisAlignedBoundingBox bounds;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        camera->far = 10.0;
        camera->near = 0.1;
        return;
    }

    RpkGeometry::listBounds(sceneGeometries, &bounds);
    camera->fitClippingPlanes(bounds);
}

/**
Renders a bounding box
*/
void
RenderOpenGL::renderBoundingBox(RpkAxisAlignedBoundingBox boundingBox) {
    Vector3Dd p[8];

    const double minX = boundingBox.minX();
    const double minY = boundingBox.minY();
    const double minZ = boundingBox.minZ();
    const double maxX = boundingBox.maxX();
    const double maxY = boundingBox.maxY();
    const double maxZ = boundingBox.maxZ();

    p[0] = Vector3Dd(minX, minY, minZ);
    p[1] = Vector3Dd(maxX, minY, minZ);
    p[2] = Vector3Dd(minX, maxY, minZ);
    p[3] = Vector3Dd(maxX, maxY, minZ);
    p[4] = Vector3Dd(minX, minY, maxZ);
    p[5] = Vector3Dd(maxX, minY, maxZ);
    p[6] = Vector3Dd(minX, maxY, maxZ);
    p[7] = Vector3Dd(maxX, maxY, maxZ);

    Opengl::openGlRenderLine(&p[0], &p[1]);
    Opengl::openGlRenderLine(&p[1], &p[3]);
    Opengl::openGlRenderLine(&p[3], &p[2]);
    Opengl::openGlRenderLine(&p[2], &p[0]);
    Opengl::openGlRenderLine(&p[4], &p[5]);
    Opengl::openGlRenderLine(&p[5], &p[7]);
    Opengl::openGlRenderLine(&p[7], &p[6]);
    Opengl::openGlRenderLine(&p[6], &p[4]);
    Opengl::openGlRenderLine(&p[0], &p[4]);
    Opengl::openGlRenderLine(&p[1], &p[5]);
    Opengl::openGlRenderLine(&p[2], &p[6]);
    Opengl::openGlRenderLine(&p[3], &p[7]);
}

void
RenderOpenGL::renderGeomBounds(Camera *camera, const RpkGeometry *geometry) {
    const RpkAxisAlignedBoundingBox geometryBoundingBox = geometry->getBoundingBox();

    if ( geometry->bounded ) {
        RenderOpenGL::renderBoundingBox(geometryBoundingBox);
    }

    if ( geometry->isCompound() ) {
        java::ArrayList<RpkGeometry *> * const geometryList = RpkGeometry::primitiveListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            RenderOpenGL::renderGeomBounds(camera, geometryList->get(i));
        }
        delete geometryList;
    }
}

/**
Renders the bounding boxes of all objects in the scene
*/
void
RenderOpenGL::renderBoundingBoxHierarchy(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries, const RendererConfiguration *renderOptions) {
    const ColorRgb &boundingBoxColor = renderOptions->getBoundingBoxColor();
    Opengl::openGlRenderSetColor(&boundingBoxColor, renderOptions);
    for ( int i = 0; sceneGeometries != nullptr && i < sceneGeometries->size(); i++ ) {
        RenderOpenGL::renderGeomBounds(camera, sceneGeometries->get(i));
    }
}

/**
Renders the cluster hierarchy bounding boxes
*/
void
RenderOpenGL::renderClusterHierarchy(Camera *camera, const java::ArrayList<RpkGeometry *> *clusteredGeometryList, const RendererConfiguration *renderOptions) {
    const ColorRgb &clusterColor = renderOptions->getClusterColor();
    Opengl::openGlRenderSetColor(&clusterColor, renderOptions);
    for ( int i = 0; clusteredGeometryList != nullptr && i < clusteredGeometryList->size(); i++ ) {
        RenderOpenGL::renderGeomBounds(camera, clusteredGeometryList->get(i));
    }
}
