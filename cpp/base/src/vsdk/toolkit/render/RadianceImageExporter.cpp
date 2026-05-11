#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/io/image/ImageOutputHandle.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/render/RadianceImageExporter.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/render/SoftIdsWrapper.h"

void
RadianceImageExporter::exportImage(
    const char *fileName,
    java::OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RendererConfiguration *renderOptions)
{
    if ( outputStream == nullptr || scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    if ( toneMapOptions == nullptr ) {
        Logger::error("RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
        return;
    }

    ScreenBuffer screenBuffer(nullptr, scene->camera, toneMapOptions);
    SoftIdsWrapper idRenderer(scene, renderOptions);

    long width;
    long height;
    idRenderer.getSize(&width, &height);
    if ( width != screenBuffer.getHRes() || height != screenBuffer.getVRes() ) {
        Logger::error("RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
        return;
    }

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer.getPatchAtPixel(x, y);
            if ( patch != nullptr ) {
                const ColorRgb radiance = RadianceImageExporter::getRadianceAtPixel(
                    &screenBuffer,
                    scene->camera,
                    x,
                    y,
                    patch,
                    radianceMethod,
                    renderOptions);
                screenBuffer.add(x, y, radiance);
            }
        }
    }

    ImageOutputHandle *imageOutputHandle = ImageOutputHandle::createRadianceImageOutputHandle(
        fileName,
        outputStream,
        isPipe,
        scene->camera->xSize,
        scene->camera->ySize);
    if ( imageOutputHandle == nullptr ) {
        return;
    }

    screenBuffer.writeFile(imageOutputHandle);
    ImageOutputHandle::deleteImageOutputHandle(imageOutputHandle);
}

void
RadianceImageExporter::clipUv(int numberOfVertices, double *u, double *v) {
    if ( *u > 1.0 - Numeric::EPSILON ) {
        *u = 1.0 - Numeric::EPSILON;
    }
    if ( *v > 1.0 - Numeric::EPSILON ) {
        *v = 1.0 - Numeric::EPSILON;
    }
    if ( numberOfVertices == 3 && (*u + *v) > 1.0 - Numeric::EPSILON ) {
        if ( *u > *v ) {
            *u = 1.0 - *v - Numeric::EPSILON;
        } else {
            *v = 1.0 - *u - Numeric::EPSILON;
        }
    }
    if ( *u < Numeric::EPSILON ) {
        *u = Numeric::EPSILON;
    }
    if ( *v < Numeric::EPSILON ) {
        *v = Numeric::EPSILON;
    }
}

ColorRgb
RadianceImageExporter::getRadianceAtPixel(
    const ScreenBuffer *screenBuffer,
    Camera *camera,
    int x,
    int y,
    Patch *patch,
    const RadianceMethod *radianceMethod,
    const RendererConfiguration *renderOptions)
{
    ColorRgb radiance{};
    radiance.clear();

    if ( screenBuffer == nullptr || camera == nullptr || patch == nullptr || radianceMethod == nullptr || renderOptions == nullptr ) {
        return radiance;
    }

    Vector3D rayDirection = screenBuffer->getPixelVector(x, y);
    rayDirection.normalize(Numeric::EPSILON_FLOAT);

    const float denominator = patch->getNormal().dotProduct(rayDirection);
    if ( denominator <= Numeric::EPSILON_FLOAT && denominator >= -Numeric::EPSILON_FLOAT ) {
        return radiance;
    }

    const float distance =
        -(patch->getNormal().dotProduct(camera->eyePosition) + patch->getPlaneConstant()) / denominator;
    Vector3D hitPoint;
    hitPoint.sumScaled(camera->eyePosition, distance, rayDirection);

    double u;
    double v;
    patch->uv(&hitPoint, &u, &v);
    RadianceImageExporter::clipUv(patch->getNumberOfVertices(), &u, &v);

    const Vector3D eyeDirection(-rayDirection.x, -rayDirection.y, -rayDirection.z);
    return radianceMethod->getRadiance(camera, patch, u, v, eyeDirection, renderOptions);
}
