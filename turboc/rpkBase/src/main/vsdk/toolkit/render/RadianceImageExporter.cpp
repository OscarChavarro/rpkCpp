#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/io/image/ImageOutputHandle.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/render/RadianceImageExporter.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/render/SoftIdsWrapper.h"

static void
updateCameraNearFarForSoftIds(const Scene *scene) {
    if ( scene == NULL || scene->camera == NULL ) {
        return;
    }

    Camera *camera = scene->camera;
    if ( scene->geometryList == NULL || scene->geometryList->size() == 0 ) {
        camera->far = 10.0;
        camera->near = 0.1;
        return;
    }

    RpkAxisAlignedBoundingBox bounds;
    Vector3Dd minimum;
    Vector3Dd maximum;
    Vector3Dd d;

    RpkGeometry::listBounds(scene->geometryList, &bounds);
    minimum = Vector3Dd(bounds.minX(), bounds.minY(), bounds.minZ());
    maximum = Vector3Dd(bounds.maxX(), bounds.maxY(), bounds.maxZ());

    camera->far = -Double::MAX_VALUE;
    camera->near = Double::MAX_VALUE;
    for ( int i = 0; i <= 1; i++ ) {
        for ( int j = 0; j <= 1; j++ ) {
            for ( int k = 0; k <= 1; k++ ) {
                d = Vector3Dd(i ? maximum.x() : minimum.x(), j ? maximum.y() : minimum.y(), k ? maximum.z() : minimum.z());

                d = d.subtract(camera->eyePosition);
                double z = d.dotProduct(camera->Z);

                if ( z > camera->far ) {
                    camera->far = z;
                }
                if ( z < camera->near ) {
                    camera->near = z;
                }
            }
        }
    }

    camera->far += 0.02 * camera->far;
    camera->near -= 0.02 * camera->near;
    if ( camera->far < VSDK::EPSILON ) {
        camera->far = camera->viewDistance;
    }
    if ( camera->near < VSDK::EPSILON ) {
        camera->near = camera->viewDistance / 100.0;
    }
}

void
RadianceImageExporter::exportImage(
    const char *fileName,
    OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions)
{
    if ( outputStream == NULL || scene == NULL || scene->camera == NULL ) {
        return;
    }

    if ( toneMapOptions == NULL ) {
        Logger::reportMessage("RadianceImageExporter", Logger::ERROR, "RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
        return;
    }

    // Match cpp behavior when OpenGL module is absent by computing near/far
    // from scene bounds before software ID rasterization.
    updateCameraNearFarForSoftIds(scene);

    ScreenBuffer screenBuffer(NULL, scene->camera, toneMapOptions);
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
            const Patch *patch = idRenderer.getPatchAtPixel(x, y);
            if ( patch != NULL ) {
                ColorRgb radiance = RadianceImageExporter::getRadianceAtPixel(
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
    if ( imageOutputHandle == NULL ) {
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

ColorRgb
RadianceImageExporter::getRadianceAtPixel(
    const ScreenBuffer *screenBuffer,
    Camera *camera,
    int x,
    int y,
    const Patch *patch,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    ColorRgb radiance = ColorRgb(0.0, 0.0, 0.0);

    if ( screenBuffer == NULL || camera == NULL || patch == NULL || radianceMethod == NULL || renderOptions == NULL ) {
        return radiance;
    }

    Vector3Dd rayDirection = screenBuffer->getPixelVector(x, y);
    rayDirection = rayDirection.normalized(Numeric::EPSILON);

    const double denominator = patch->normal.dotProduct(rayDirection);
    if ( denominator <= Numeric::EPSILON && denominator >= -Numeric::EPSILON ) {
        return radiance;
    }

    const double distance =
        -(patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant) / denominator;
    Vector3Dd hitPoint;
    hitPoint = camera->eyePosition.add(rayDirection.multiply(((double)(distance))));

    double u;
    double v;
    patch->uv(&hitPoint, &u, &v);
    RadianceImageExporter::clipUv(patch->numberOfVertices, &u, &v);

    const Vector3Dd eyeDirection(-rayDirection.x(), -rayDirection.y(), -rayDirection.z());
    return ColorRgb(radianceMethod->getRadiance(camera, patch, u, v, eyeDirection, renderOptions));
}
