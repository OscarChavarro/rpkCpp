/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "render/opengl.h"
#include "render/render.h"
#include "render/softids.h"

/**
Sets up a software rendering context and initialises transforms and
viewport for the current view. The new renderer is made current
*/
SGL_CONTEXT *
setupSoftFrameBuffer(const Camera *camera) {
    SGL_CONTEXT *sgl = new SGL_CONTEXT(camera->xSize, camera->ySize);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);
    GLOBAL_sgl_currentContext->sglClipping(true);
    GLOBAL_sgl_currentContext->sglClear((SGL_PIXEL) 0, SGL_MAXIMUM_Z);

    Matrix4x4 p = perspectiveMatrix(
            camera->fieldOfVision * 2.0f * (float)M_PI / 180.0f,
            (float) camera->xSize / (float) camera->ySize,
            camera->near,
            camera->far);
    GLOBAL_sgl_currentContext->sglLoadMatrix(&p);
    Matrix4x4 l = lookAtMatrix(camera->eyePosition, camera->lookPosition, camera->upDirection);
    GLOBAL_sgl_currentContext->sglMultiplyMatrix(&l);

    return sgl;
}

static void
softRenderPatch(Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
    Vector3D vertices[4];

    if ( renderOptions->backfaceCulling &&
        patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant < EPSILON ) {
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
softRenderPatches(const Scene *scene, const RenderOptions *renderOptions) {
    if ( renderOptions->frustumCulling ) {
        openGlRenderWorldOctree(scene, softRenderPatch, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            softRenderPatch(scene->patchList->get(i), scene->camera, renderOptions);
        }
    }
}

/**
Software ID rendering
*/
unsigned long *
softRenderIds(long *x, long *y, const Scene *scene, const RenderOptions *renderOptions) {
    SGL_CONTEXT *currentSglContext;
    SGL_CONTEXT *oldSglContext;
    unsigned long *ids;

    oldSglContext = GLOBAL_sgl_currentContext;
    currentSglContext = setupSoftFrameBuffer(scene->camera);
    softRenderPatches(scene, renderOptions);

    *x = currentSglContext->width;
    *y = currentSglContext->height;
    ids = (unsigned long *)malloc((int) (*x) * (int) (*y) * sizeof(unsigned long));
    memcpy(ids, currentSglContext->frameBuffer, currentSglContext->width * currentSglContext->height * sizeof(long));

    delete currentSglContext;
    sglMakeCurrent(oldSglContext);

    return ids;
}
