#include "vsdk/toolkit/common/VSDK.h"
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
        Logger::reportMessage("RadianceImageExporter", Logger::ERROR, "RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
        return;
    }

    ScreenBuffer screenBuffer(nullptr, scene->camera, toneMapOptions);
    SoftIdsWrapper idRenderer(scene, renderOptions);

    long width;
    long height;
    idRenderer.getSize(&width, &height);
    if ( width != screenBuffer.getHRes() || height != screenBuffer.getVRes() ) {
        Logger::reportMessage("RadianceImageExporter", Logger::ERROR, "RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
        return;
    }

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer.getPatchAtPixel(x, y);
            if ( patch != nullptr ) {
                const ColorRgbMutable radiance = RadianceImageExporter::getRadianceAtPixel(
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
    if ( *u > 1.0 - VSDK::EPSILON ) {
        *u = 1.0 - VSDK::EPSILON;
    }
    if ( *v > 1.0 - VSDK::EPSILON ) {
        *v = 1.0 - VSDK::EPSILON;
    }
    if ( numberOfVertices == 3 && (*u + *v) > 1.0 - VSDK::EPSILON ) {
        if ( *u > *v ) {
            *u = 1.0 - *v - VSDK::EPSILON;
        } else {
            *v = 1.0 - *u - VSDK::EPSILON;
        }
    }
    if ( *u < VSDK::EPSILON ) {
        *u = VSDK::EPSILON;
    }
    if ( *v < VSDK::EPSILON ) {
        *v = VSDK::EPSILON;
    }
}

ColorRgbMutable
RadianceImageExporter::getRadianceAtPixel(
    const ScreenBuffer *screenBuffer,
    Camera *camera,
    int x,
    int y,
    Patch *patch,
    const RadianceMethod *radianceMethod,
    const RendererConfiguration *renderOptions)
{
    ColorRgbMutable radiance{};
    radiance.clear();

    if ( screenBuffer == nullptr || camera == nullptr || patch == nullptr || radianceMethod == nullptr || renderOptions == nullptr ) {
        return radiance;
    }

    Vector3Df rayDirection = screenBuffer->getPixelVector(x, y);
    rayDirection = rayDirection.normalized(Numeric::EPSILON_FLOAT);

    const float denominator = patch->getNormal().dotProduct(rayDirection);
    if ( denominator <= Numeric::EPSILON_FLOAT && denominator >= -Numeric::EPSILON_FLOAT ) {
        return radiance;
    }

    const float distance =
        -(patch->getNormal().dotProduct(camera->eyePosition) + patch->getPlaneConstant()) / denominator;
    Vector3Df hitPoint;
    hitPoint = camera->eyePosition.add(rayDirection.multiply(static_cast<float>(distance)));

    double u;
    double v;
    patch->uv(&hitPoint, &u, &v);
    RadianceImageExporter::clipUv(patch->getNumberOfVertices(), &u, &v);

    const Vector3Df eyeDirection(-rayDirection.x(), -rayDirection.y(), -rayDirection.z());
    return radianceMethod->getRadiance(camera, patch, u, v, eyeDirection, renderOptions);
}
