/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <string.h>

#include "java/util/ArrayList.txx"
#include "tonemap/ToneMap.h"
#include "render/Softids.h"

/**
Sets up a software rendering context and initialises transforms and
viewport for the current view.
*/
SglContext *
SoftIds::setupSoftFrameBuffer(const Camera *camera) {
    SglContext *sgl = new SglContext(camera->xSize, camera->ySize);
    sgl->sglDepthTesting(true);
    sgl->sglClipping(true);
    sgl->sglClear(0, SglConstants::SGL_MAXIMUM_Z);

    Matrix4x4 p = Matrix4x4::createPerspectiveMatrix(
        camera->fieldOfVision * 2.0f * ((float)(M_PI)) / 180.0f,
        ((float)(camera->xSize)) / ((float)(camera->ySize)),
        camera->near,
        camera->far);
    sgl->sglLoadMatrix(&p);
    Matrix4x4 l = Matrix4x4::createLookAtMatrix(camera->eyePosition, camera->lookPosition, camera->upDirection);
    sgl->sglMultiplyMatrix(&l);

    return sgl;
}

void
SoftIds::softRenderPatch(
    const Patch *patch,
    const Camera *camera,
    const RenderOptions *renderOptions,
    SglContext *sglContext)
{
    if ( patch == NULL || camera == NULL || renderOptions == NULL || sglContext == NULL ) {
        return;
    }

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

    sglContext->sglSetPatch(patch);
    sglContext->sglPolygon(patch->numberOfVertices, vertices);
}

void
SoftIds::softRenderPatches(const Scene *scene, const RenderOptions *renderOptions, SglContext *sglContext) {
    if ( scene == NULL || renderOptions == NULL || sglContext == NULL ) {
        return;
    }

    for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
        SoftIds::softRenderPatch(scene->patchList->get(i), scene->camera, renderOptions, sglContext);
    }
}

/**
Software ID rendering

Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
*/
unsigned long *
SoftIds::softRenderIds(long *x, long *y, const Scene *scene, const RenderOptions *renderOptions) {
    SglContext *currentSglContext = SoftIds::setupSoftFrameBuffer(scene->camera);
    SoftIds::softRenderPatches(scene, renderOptions, currentSglContext);

    *x = currentSglContext->width;
    *y = currentSglContext->height;
    unsigned long *ids = new unsigned long[currentSglContext->width * currentSglContext->height];
    memcpy(ids, currentSglContext->frameBuffer, currentSglContext->width * currentSglContext->height * sizeof(unsigned long));

    delete currentSglContext;

    return ids;
}

/**
Renders in memory an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
void
SoftIds::softRenderPixels(int width, int height, const ColorRgb *rgb, const ToneMappingContext &toneMapOptions) {
    // Length of one row of RGBA image data rounded up to a multiple of 8
    const int rowLength = ((int)((4 * width * sizeof(unsigned char) + 7) & ~7));
    unsigned char *c = new unsigned char[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        const int rowRgbStart = j * width;
        const int rowStart = j * rowLength;
        for ( int i = 0; i < width; i++ ) {
            ColorRgb corrected_rgb = rgb[rowRgbStart + i];
            ToneMap::toneMappingGammaCorrection(corrected_rgb, toneMapOptions);
            const int pixelOffset = rowStart + 4 * i;
            c[pixelOffset] = ((unsigned char)(corrected_rgb.r * 255.0));
            c[pixelOffset + 1] = ((unsigned char)(corrected_rgb.g * 255.0));
            c[pixelOffset + 2] = ((unsigned char)(corrected_rgb.b * 255.0));
            c[pixelOffset + 3] = 255; // alpha = 1.0
        }
    }

    delete[] c;
}
