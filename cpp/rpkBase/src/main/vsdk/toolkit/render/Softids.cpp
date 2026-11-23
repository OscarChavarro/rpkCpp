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
    const RendererConfiguration &renderOptions,
    FaceVisibilityBuffer &target)
{
    if ( renderOptions.isBackfaceCulling() &&
         patch.getNormal().dotProduct(camera.eyePosition) + patch.getPlaneConstant() < VSDK::EPSILON ) {
        return;
    }

    Vector3Df vertices[4];
    const int count = patch.getNumberOfVertices();
    for ( int i = 0; i < count; i++ ) {
        vertices[i] = *patch.getVertices()[i]->point;
    }
    target.drawPolygon(count, vertices, SglFaceIndex(patch.getFaceIndex()));
}

void
SoftIds::renderVisibleFaces(const Scene &scene, const RendererConfiguration *renderOptions, FaceVisibilityBuffer &target) {
    if ( renderOptions == nullptr ) {
        return;
    }

    const Camera &camera = *scene.camera;
    const Matrix4x4f projection = Camera::perspectiveMatrix(
        camera.fieldOfVision * 2.0F * static_cast<float>(M_PI) / 180.0F,
        static_cast<float>(camera.xSize) / static_cast<float>(camera.ySize),
        camera.near,
        camera.far);
    const Matrix4x4f view = Camera::lookAtMatrix(camera.eyePosition, camera.lookPosition, camera.upDirection);
    target.setTransform(projection.multiply(view));

    for ( int i = 0; scene.patchList != nullptr && i < scene.patchList->size(); i++ ) {
        const Patch *patch = scene.patchList->get(i);
        if ( patch != nullptr ) {
            drawPatch(*patch, camera, *renderOptions, target);
        }
    }
}

/**
Legacy patch ID export. Zero represents the background.
*/
unsigned long *
SoftIds::softRenderIds(long *width, long *height, const Scene *scene, const RendererConfiguration *renderOptions) {
    FaceVisibilityBuffer target(scene->camera->xSize, scene->camera->ySize, SglFaceIndex());
    renderVisibleFaces(*scene, renderOptions, target);

    *width = target.getWidth();
    *height = target.getHeight();
    unsigned long *const ids = new unsigned long[*width * *height];
    for ( int y = 0; y < *height; y++ ) {
        const long rowStart = y * *width;
        for ( int x = 0; x < *width; x++ ) {
            const SglFaceIndex face = target.getSample(x, y);
            ids[rowStart + x] = face.isBackground() ? 0 : scene->getFace(face.getIndex())->getId();
        }
    }
    return ids;
}

/**
Renders in memory an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
void
SoftIds::softRenderPixels(int width, int height, const ColorRgbMutable *rgb, const ToneMappingContext &toneMapOptions) {
    // Length of one row of RGBA image data rounded up to a multiple of 8
    const int rowLength = static_cast<int>((4 * width * sizeof(unsigned char) + 7) & ~7);
    unsigned char * const c = new unsigned char[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        const int rowRgbStart = j * width;
        const int rowStart = j * rowLength;
        for ( int i = 0; i < width; i++ ) {
            ColorRgbMutable corrected_rgb = rgb[rowRgbStart + i];
            ToneMap::toneMappingGammaCorrection(corrected_rgb, toneMapOptions);
            const int pixelOffset = rowStart + 4 * i;
            c[pixelOffset] = static_cast<unsigned char>(corrected_rgb.getR() * 255.0);
            c[pixelOffset + 1] = static_cast<unsigned char>(corrected_rgb.getG() * 255.0);
            c[pixelOffset + 2] = static_cast<unsigned char>(corrected_rgb.getB() * 255.0);
            c[pixelOffset + 3] = 255; // alpha = 1.0
        }
    }

    delete[] c;
}
