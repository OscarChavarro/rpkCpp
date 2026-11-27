/**
Software ID rendering: because hardware ID rendering is tricky due to frame buffer
formats, etc.
*/

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/render/Softids.h"

void
SoftIds::drawPatch(
    const Patch &patch,
    const Camera &camera,
    const RenderOptions &renderOptions,
    FaceVisibilityBuffer &target)
{
    if ( renderOptions.isBackfaceCulling() &&
         patch.normal.dotProduct(camera.eyePosition) + patch.planeConstant < VSDK::EPSILON ) {
        return;
    }

    Vector3Dd vertices[4];
    const int count = patch.numberOfVertices;
    for ( int i = 0; i < count; i++ ) {
        vertices[i] = *patch.vertex[i]->point;
    }
    target.drawPolygon(count, vertices, FaceIndexSample(patch.getFaceIndex()));
}

void
SoftIds::renderVisibleFaces(const Scene &scene, const RenderOptions *renderOptions, FaceVisibilityBuffer &target) {
    if ( renderOptions == NULL ) {
        return;
    }

    const Camera &camera = *scene.camera;
    const Matrix4x4d projection = Camera::perspectiveMatrix(
        camera.fieldOfVision * 2.0 * ((double)(M_PI)) / 180.0,
        ((double)(camera.xSize)) / ((double)(camera.ySize)),
        camera.near,
        camera.far);
    const Matrix4x4d view = Camera::lookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
    target.setTransform(projection.multiply(view));

    for ( int i = 0; scene.patchList != NULL && i < scene.patchList->size(); i++ ) {
        const Patch *patch = scene.patchList->get(i);
        if ( patch != NULL ) {
            drawPatch(*patch, camera, *renderOptions, target);
        }
    }
}

/**
Legacy patch ID export. Zero represents the background.
*/
unsigned long *
SoftIds::softRenderIds(long *width, long *height, const Scene *scene, const RenderOptions *renderOptions) {
    FaceVisibilityBuffer target(scene->camera->xSize, scene->camera->ySize, FaceIndexSample());
    renderVisibleFaces(*scene, renderOptions, target);

    *width = target.getWidth();
    *height = target.getHeight();
    unsigned long *const ids = new unsigned long[*width * *height];
    for ( int y = 0; y < *height; y++ ) {
        const long rowStart = y * *width;
        for ( int x = 0; x < *width; x++ ) {
            const FaceIndexSample face = target.getSample(x, y);
            ids[rowStart + x] = face.isBackground() ? 0 : scene->getFace(face.getIndex())->id;
        }
    }
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
            c[pixelOffset] = ((unsigned char)(corrected_rgb.getR() * 255.0));
            c[pixelOffset + 1] = ((unsigned char)(corrected_rgb.getG() * 255.0));
            c[pixelOffset + 2] = ((unsigned char)(corrected_rgb.getB() * 255.0));
            c[pixelOffset + 3] = 255; // alpha = 1.0
        }
    }

    delete[] c;
}
