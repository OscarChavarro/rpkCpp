/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "render/opengl.h"
#include "render/render.h"
#include "render/softids.h"
#include "tonemap/ToneMap.h"

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

    Matrix4x4 p = Matrix4x4::createPerspectiveMatrix(
        camera->fieldOfVision * 2.0f * (float) M_PI / 180.0f,
        (float) camera->xSize / (float) camera->ySize,
        camera->near,
        camera->far);
    GLOBAL_sgl_currentContext->sglLoadMatrix(&p);
    Matrix4x4 l = Matrix4x4::createLookAtMatrix(camera->eyePosition, camera->lookPosition, camera->upDirection);
    GLOBAL_sgl_currentContext->sglMultiplyMatrix(&l);

    return sgl;
}

static void
softRenderPatch(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
    Vector3D vertices[4];

    if ( renderOptions->backfaceCulling &&
        patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant < Numeric::EPSILON ) {
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

Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
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
    ids = new unsigned long[currentSglContext->width * currentSglContext->height];
    memcpy(ids, currentSglContext->frameBuffer, currentSglContext->width * currentSglContext->height * sizeof(unsigned long));

    delete currentSglContext;
    sglMakeCurrent(oldSglContext);

    return ids;
}

/**
Renders in memory an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
void
softRenderPixels(int width, int height, const ColorRgb *rgb) {
    int rowLength;

    // Length of one row of RGBA image data rounded up to a multiple of 8
    rowLength = (int)((4 * width * sizeof(unsigned char) + 7) & ~7);
    unsigned char *c = new unsigned char[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        const ColorRgb *rgbP = &rgb[j * width];

        unsigned char *p = c + j * rowLength; // Let each line start on an 8-byte boundary
        for ( int i = 0; i < width; i++, rgbP++ ) {
            ColorRgb corrected_rgb = *rgbP;
            toneMappingGammaCorrection(corrected_rgb);
            *p++ = (unsigned char) (corrected_rgb.r * 255.0);
            *p++ = (unsigned char) (corrected_rgb.g * 255.0);
            *p++ = (unsigned char) (corrected_rgb.b * 255.0);
            *p++ = 255; // alpha = 1.0
        }
    }

    delete[] c;
}
