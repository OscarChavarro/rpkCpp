/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>
#include <cstdlib>

#include "shared/softids.h"
#include "scene/scene.h"
#include "shared/Camera.h"
#include "shared/render.h"
#include "common/error.h"

SGL_CONTEXT *
SetupSoftFrameBuffer() {
    SGL_CONTEXT *sgl;

    sgl = sglOpen(GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize);
    sglDepthTesting(true);
    sglClipping(true);
    sglClear((SGL_PIXEL) 0, SGL_MAXIMUM_Z);

    sglLoadMatrix(perspectiveMatrix(GLOBAL_camera_mainCamera.fov * 2. * M_PI / 180.,
                                    (float) GLOBAL_camera_mainCamera.xSize / (float) GLOBAL_camera_mainCamera.ySize,
                                    GLOBAL_camera_mainCamera.near,
                                    GLOBAL_camera_mainCamera.far));
    sglMultMatrix(lookAtMatrix(GLOBAL_camera_mainCamera.eyePosition, GLOBAL_camera_mainCamera.lookPosition,
                               GLOBAL_camera_mainCamera.upDirection));

    return sgl;
}

static SGL_PIXEL (*PatchPixel)(Patch *) = nullptr;

static void
SoftRenderPatch(Patch *P) {
    Vector3D verts[4];

    if ( GLOBAL_render_renderOptions.backface_culling &&
         VECTORDOTPRODUCT(P->normal, GLOBAL_camera_mainCamera.eyePosition) + P->planeConstant < EPSILON ) {
        return;
    }

    verts[0] = *P->vertex[0]->point;
    verts[1] = *P->vertex[1]->point;
    verts[2] = *P->vertex[2]->point;
    if ( P->numberOfVertices > 3 ) {
        verts[3] = *P->vertex[3]->point;
    }

    sglSetColor(PatchPixel(P));
    sglPolygon(P->numberOfVertices, verts);
}

void
SoftRenderPatches(SGL_PIXEL (*patch_pixel)(Patch *)) {
    PatchPixel = patch_pixel;

    if ( GLOBAL_render_renderOptions.frustum_culling ) {
        int use_display_lists = GLOBAL_render_renderOptions.use_display_lists;
        GLOBAL_render_renderOptions.use_display_lists = false;  /* temporarily switch it off */
        renderWorldOctree(SoftRenderPatch);
        GLOBAL_render_renderOptions.use_display_lists = use_display_lists;
    } else {
        for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
            SoftRenderPatch(window->patch);
        }
    }
}

static SGL_PIXEL
PatchID(Patch *P) {
    return (SGL_PIXEL) P->id;
}

static void
SoftRenderPatchIds() {
    SoftRenderPatches(PatchID);
}

unsigned long *
SoftRenderIds(long *x, long *y) {
    SGL_CONTEXT *sgl, *oldsgl;
    unsigned long *ids;

    if ( sizeof(SGL_PIXEL) != sizeof(long)) {
        logFatal(-1, "SoftRenderIds", "sizeof(SGL_PIXEL)!=sizeof(long).");
    }

    oldsgl = GLOBAL_sgl_currentContext;
    sgl = SetupSoftFrameBuffer();
    SoftRenderPatchIds();

    *x = sgl->width;
    *y = sgl->height;
    ids = (unsigned long *)malloc((int) (*x) * (int) (*y) * sizeof(unsigned long));
    memcpy(ids, sgl->frameBuffer, sgl->width * sgl->height * sizeof(long));

    sglClose(sgl);
    sglMakeCurrent(oldsgl);

    return ids;
}
