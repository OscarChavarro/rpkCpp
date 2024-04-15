/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "render/opengl.h"
#include "render/render.h"
#include "render/softids.h"

/**
Sets up a software rendering context and initialises transforms and
viewport for the current view. The new renderer is made current
*/
SGL_CONTEXT *
setupSoftFrameBuffer() {
    SGL_CONTEXT *sgl = new SGL_CONTEXT(GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);
    GLOBAL_sgl_currentContext->sglClipping(true);
    GLOBAL_sgl_currentContext->sglClear((SGL_PIXEL) 0, SGL_MAXIMUM_Z);

    GLOBAL_sgl_currentContext->sglLoadMatrix(perspectiveMatrix(
        GLOBAL_camera_mainCamera.fov * 2.0f * (float)M_PI / 180.0f,
            (float) GLOBAL_camera_mainCamera.xSize / (float) GLOBAL_camera_mainCamera.ySize,
            GLOBAL_camera_mainCamera.near,
            GLOBAL_camera_mainCamera.far));
    GLOBAL_sgl_currentContext->sglMultiplyMatrix(lookAtMatrix(
            GLOBAL_camera_mainCamera.eyePosition, GLOBAL_camera_mainCamera.lookPosition,
            GLOBAL_camera_mainCamera.upDirection));

    return sgl;
}

static void
softRenderPatch(Patch *patch, Camera * /*camera*/) {
    Vector3D vertices[4];

    if ( GLOBAL_render_renderOptions.backfaceCulling &&
            vectorDotProduct(patch->normal, GLOBAL_camera_mainCamera.eyePosition) + patch->planeConstant < EPSILON ) {
        return;
    }

    vertices[0] = *patch->vertex[0]->point;
    vertices[1] = *patch->vertex[1]->point;
    vertices[2] = *patch->vertex[2]->point;
    if ( patch->numberOfVertices > 3 ) {
        vertices[3] = *patch->vertex[3]->point;
    }

    GLOBAL_sgl_currentContext->sglSetPatch(patch);
    GLOBAL_sgl_currentContext->sglPolygon(patch->numberOfVertices, vertices);
}

/**
Renders all scenePatches in the current sgl renderer. PatchPixel returns
and SGL_PIXEL value for a given Patch
*/
void
softRenderPatches(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    if ( GLOBAL_render_renderOptions.frustumCulling ) {
        char use_display_lists = GLOBAL_render_renderOptions.useDisplayLists;
        GLOBAL_render_renderOptions.useDisplayLists = false;  // Temporarily switch it off
        openGlRenderWorldOctree(camera, softRenderPatch, clusteredWorldGeometry);
        GLOBAL_render_renderOptions.useDisplayLists = use_display_lists;
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            softRenderPatch(scenePatches->get(i), camera);
        }
    }
}

/**
Software ID rendering
*/
unsigned long *
softRenderIds(
    long *x,
    long *y,
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    SGL_CONTEXT *currentSglContext;
    SGL_CONTEXT *oldSglContext;
    unsigned long *ids;

    if ( sizeof(SGL_PIXEL) != sizeof(long) ) {
        logFatal(-1, "softRenderIds", "sizeof(SGL_PIXEL)!=sizeof(long).");
    }

    oldSglContext = GLOBAL_sgl_currentContext;
    currentSglContext = setupSoftFrameBuffer();
    softRenderPatches(camera, scenePatches, clusteredWorldGeometry);

    *x = currentSglContext->width;
    *y = currentSglContext->height;
    ids = (unsigned long *)malloc((int) (*x) * (int) (*y) * sizeof(unsigned long));
    memcpy(ids, currentSglContext->frameBuffer, currentSglContext->width * currentSglContext->height * sizeof(long));

    delete currentSglContext;
    sglMakeCurrent(oldSglContext);

    return ids;
}
