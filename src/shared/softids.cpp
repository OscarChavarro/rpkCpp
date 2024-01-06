/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>
#include <cstdlib>

#include "shared/softids.h"
#include "scene/scene.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "common/error.h"

SGL_CONTEXT *
SetupSoftFrameBuffer() {
    SGL_CONTEXT *sgl;

    sgl = sglOpen(GLOBAL_camera_mainCamera.hres, GLOBAL_camera_mainCamera.vres);
    sglDepthTesting(true);
    sglClipping(true);
    sglClear((SGL_PIXEL) 0, SGL_ZMAX);

    sglLoadMatrix(Perspective(GLOBAL_camera_mainCamera.fov * 2. * M_PI / 180., (float) GLOBAL_camera_mainCamera.hres / (float) GLOBAL_camera_mainCamera.vres, GLOBAL_camera_mainCamera.near,
                              GLOBAL_camera_mainCamera.far));
    sglMultMatrix(LookAt(GLOBAL_camera_mainCamera.eyep, GLOBAL_camera_mainCamera.lookp, GLOBAL_camera_mainCamera.updir));

    return sgl;
}

static SGL_PIXEL (*PatchPixel)(PATCH *) = nullptr;

static void
SoftRenderPatch(PATCH *P) {
    Vector3D verts[4];

    if ( renderopts.backface_culling &&
         VECTORDOTPRODUCT(P->normal, GLOBAL_camera_mainCamera.eyep) + P->plane_constant < EPSILON ) {
        return;
    }

    verts[0] = *P->vertex[0]->point;
    verts[1] = *P->vertex[1]->point;
    verts[2] = *P->vertex[2]->point;
    if ( P->nrvertices > 3 ) {
        verts[3] = *P->vertex[3]->point;
    }

    sglSetColor(PatchPixel(P));
    sglPolygon(P->nrvertices, verts);
}

void
SoftRenderPatches(SGL_PIXEL (*patch_pixel)(PATCH *)) {
    PatchPixel = patch_pixel;

    if ( renderopts.frustum_culling ) {
        int use_display_lists = renderopts.use_display_lists;
        renderopts.use_display_lists = false;  /* temporarily switch it off */
        RenderWorldOctree(SoftRenderPatch);
        renderopts.use_display_lists = use_display_lists;
    } else {
        ForAllPatches(P, GLOBAL_scene_patches)
                    {
                        SoftRenderPatch(P);
                    }
        EndForAll;
    }
}

static SGL_PIXEL
PatchID(PATCH *P) {
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

    oldsgl = sglGetCurrent();
    sgl = SetupSoftFrameBuffer();
    SoftRenderPatchIds();

    *x = sgl->width;
    *y = sgl->height;
    ids = (unsigned long *)malloc((int) (*x) * (int) (*y) * sizeof(unsigned long));
    memcpy(ids, sgl->fbuf, sgl->width * sgl->height * sizeof(long));

    sglClose(sgl);
    sglMakeCurrent(oldsgl);

    return ids;
}
