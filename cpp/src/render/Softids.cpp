/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "tonemap/ToneMap.h"
#include "render/Softids.h"

/**
Sets up a software rendering context and initialises transforms and
viewport for the current view.
*/
SglContext *
SoftIds::setupSoftFrameBuffer(const Camera *camera) {
    SglContext * const sgl = new SglContext(camera->xSize, camera->ySize);
    sgl->sglDepthTesting(true);
    sgl->sglClipping(true);
    sgl->sglClear(0, SglConstants::SGL_MAXIMUM_Z);

    const Matrix4x4 p = Matrix4x4::createPerspectiveMatrix(
        camera->fieldOfVision * 2.0f * static_cast<float>(M_PI) / 180.0f,
        static_cast<float>(camera->xSize) / static_cast<float>(camera->ySize),
        camera->near,
        camera->far);
    sgl->sglLoadMatrix(&p);
    const Matrix4x4 l = Matrix4x4::createLookAtMatrix(camera->eyePosition, camera->lookPosition, camera->upDirection);
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
    if ( patch == nullptr || camera == nullptr || renderOptions == nullptr || sglContext == nullptr ) {
        return;
    }

    Vector3D vertices[4];

    if ( renderOptions->backfaceCulling &&
         patch->getNormal().dotProduct(camera->eyePosition) + patch->getPlaneConstant() < Numeric::EPSILON ) {
        return;
    }

    vertices[0] = *patch->getVertices()[0]->point;
    vertices[1] = *patch->getVertices()[1]->point;
    vertices[2] = *patch->getVertices()[2]->point;
    if ( patch->getNumberOfVertices() > 3 ) {
        vertices[3] = *patch->getVertices()[3]->point;
    }

    sglContext->sglSetPatch(patch);
    sglContext->sglPolygon(patch->getNumberOfVertices(), vertices);
}

void
SoftIds::softRenderPatches(const Scene *scene, const RenderOptions *renderOptions, SglContext *sglContext) {
    if ( scene == nullptr || renderOptions == nullptr || sglContext == nullptr ) {
        return;
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
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
    SglContext * const currentSglContext = SoftIds::setupSoftFrameBuffer(scene->camera);
    SoftIds::softRenderPatches(scene, renderOptions, currentSglContext);

    *x = currentSglContext->width;
    *y = currentSglContext->height;
    unsigned long * const ids = new unsigned long[currentSglContext->width * currentSglContext->height];
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
    const int rowLength = static_cast<int>((4 * width * sizeof(unsigned char) + 7) & ~7);
    unsigned char * const c = new unsigned char[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        const int rowRgbStart = j * width;
        const int rowStart = j * rowLength;
        for ( int i = 0; i < width; i++ ) {
            ColorRgb corrected_rgb = rgb[rowRgbStart + i];
            ToneMap::toneMappingGammaCorrection(corrected_rgb, toneMapOptions);
            const int pixelOffset = rowStart + 4 * i;
            c[pixelOffset] = static_cast<unsigned char>(corrected_rgb.r * 255.0);
            c[pixelOffset + 1] = static_cast<unsigned char>(corrected_rgb.g * 255.0);
            c[pixelOffset + 2] = static_cast<unsigned char>(corrected_rgb.b * 255.0);
            c[pixelOffset + 3] = 255; // alpha = 1.0
        }
    }

    delete[] c;
}
