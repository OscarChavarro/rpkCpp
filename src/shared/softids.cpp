/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "shared/render.h"
#include "shared/softids.h"

/**
Sets up a software rendering context and initialises transforms and
viewport for the current view. The new renderer is made current
*/
SGL_CONTEXT *
setupSoftFrameBuffer() {
    SGL_CONTEXT *sgl;

    sgl = sglOpen(GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize);
    sglDepthTesting(true);
    sglClipping(true);
    sglClear((SGL_PIXEL) 0, SGL_MAXIMUM_Z);

    sglLoadMatrix(perspectiveMatrix(
        GLOBAL_camera_mainCamera.fov * 2.0f * (float)M_PI / 180.0f,
            (float) GLOBAL_camera_mainCamera.xSize / (float) GLOBAL_camera_mainCamera.ySize,
            GLOBAL_camera_mainCamera.near,
            GLOBAL_camera_mainCamera.far));
    sglMultiplyMatrix(lookAtMatrix(
            GLOBAL_camera_mainCamera.eyePosition, GLOBAL_camera_mainCamera.lookPosition,
            GLOBAL_camera_mainCamera.upDirection));

    return sgl;
}

static SGL_PIXEL (*PatchPixel)(Patch *) = nullptr;

static void
softRenderPatch(Patch *P) {
    Vector3D vertices[4];

    if ( GLOBAL_render_renderOptions.backface_culling &&
         VECTORDOTPRODUCT(P->normal, GLOBAL_camera_mainCamera.eyePosition) + P->planeConstant < EPSILON ) {
        return;
    }

    vertices[0] = *P->vertex[0]->point;
    vertices[1] = *P->vertex[1]->point;
    vertices[2] = *P->vertex[2]->point;
    if ( P->numberOfVertices > 3 ) {
        vertices[3] = *P->vertex[3]->point;
    }

    sglSetColor(PatchPixel(P));
    sglPolygon(P->numberOfVertices, vertices);
}

/**
Renders all scenePatches in the current sgl renderer. PatchPixel returns
and SGL_PIXEL value for a given Patch
*/
void
softRenderPatches(
    SGL_PIXEL (*patch_pixel)(Patch *),
    java::ArrayList<Patch *> *scenePatches)
{
    PatchPixel = patch_pixel;

    if ( GLOBAL_render_renderOptions.frustum_culling ) {
        char use_display_lists = GLOBAL_render_renderOptions.use_display_lists;
        GLOBAL_render_renderOptions.use_display_lists = false;  /* temporarily switch it off */
        renderWorldOctree(softRenderPatch);
        GLOBAL_render_renderOptions.use_display_lists = use_display_lists;
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            softRenderPatch(scenePatches->get(i));
        }
    }
}

static SGL_PIXEL
patchId(Patch *P) {
    return (SGL_PIXEL) P->id;
}

static void
softRenderPatchIds(java::ArrayList<Patch *> *scenePatches) {
    softRenderPatches(patchId, scenePatches);
}

/**
Software ID rendering
*/
unsigned long *
softRenderIds(long *x, long *y, java::ArrayList<Patch *> *scenePatches) {
    SGL_CONTEXT *currentSglContext;
    SGL_CONTEXT *oldSglContext;
    unsigned long *ids;

    if ( sizeof(SGL_PIXEL) != sizeof(long) ) {
        logFatal(-1, "softRenderIds", "sizeof(SGL_PIXEL)!=sizeof(long).");
    }

    oldSglContext = GLOBAL_sgl_currentContext;
    currentSglContext = setupSoftFrameBuffer();
    softRenderPatchIds(scenePatches);

    *x = currentSglContext->width;
    *y = currentSglContext->height;
    ids = (unsigned long *)malloc((int) (*x) * (int) (*y) * sizeof(unsigned long));
    memcpy(ids, currentSglContext->frameBuffer, currentSglContext->width * currentSglContext->height * sizeof(long));

    sglClose(currentSglContext);
    sglMakeCurrent(oldSglContext);

    return ids;
}
