#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "common/linealAlgebra/Numeric.h"
#include "io/image/ImageOutputHandle.h"
#include "scene/RadianceMethod.h"
#include "render/RadianceImageExporter.h"
#include "render/ScreenBuffer.h"
#include "render/SoftIdsWrapper.h"

static void
updateCameraNearFarForSoftIds(const Scene *scene) {
    if ( scene == NULL || scene->camera == NULL ) {
        return;
    }

    Camera *camera = scene->camera;
    if ( scene->geometryList == NULL || scene->geometryList->size() == 0 ) {
        camera->far = 10.0f;
        camera->near = 0.1f;
        return;
    }

    BoundingBox bounds;
    Vector3D minimum;
    Vector3D maximum;
    Vector3D d;

    Geometry::listBounds(scene->geometryList, &bounds);
    minimum.set(bounds.minX(), bounds.minY(), bounds.minZ());
    maximum.set(bounds.maxX(), bounds.maxY(), bounds.maxZ());

    camera->far = -Numeric::HUGE_FLOAT_VALUE;
    camera->near = Numeric::HUGE_FLOAT_VALUE;
    for ( int i = 0; i <= 1; i++ ) {
        for ( int j = 0; j <= 1; j++ ) {
            for ( int k = 0; k <= 1; k++ ) {
                d.set(i ? maximum.x : minimum.x,
                      j ? maximum.y : minimum.y,
                      k ? maximum.z : minimum.z);

                d.subtraction(d, camera->eyePosition);
                float z = d.dotProduct(camera->Z);

                if ( z > camera->far ) {
                    camera->far = z;
                }
                if ( z < camera->near ) {
                    camera->near = z;
                }
            }
        }
    }

    camera->far += 0.02f * camera->far;
    camera->near -= 0.02f * camera->near;
    if ( camera->far < Numeric::EPSILON ) {
        camera->far = camera->viewDistance;
    }
    if ( camera->near < Numeric::EPSILON ) {
        camera->near = camera->viewDistance / 100.0f;
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
        Logger::error("RadianceImageExporter::exportImage", "Tone mapping context not provided for image export");
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
        Logger::error("RadianceImageExporter::exportImage", "ID buffer size does not match screen size");
        return;
    }

    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer.getPatchAtPixel(x, y);
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
    const RenderOptions *renderOptions)
{
    ColorRgb radiance = ColorRgb(0.0f, 0.0f, 0.0f);

    if ( screenBuffer == NULL || camera == NULL || patch == NULL || radianceMethod == NULL || renderOptions == NULL ) {
        return radiance;
    }

    Vector3D rayDirection = screenBuffer->getPixelVector(x, y);
    rayDirection.normalize(Numeric::EPSILON_FLOAT);

    const float denominator = patch->normal.dotProduct(rayDirection);
    if ( denominator <= Numeric::EPSILON_FLOAT && denominator >= -Numeric::EPSILON_FLOAT ) {
        return radiance;
    }

    const float distance =
        -(patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant) / denominator;
    Vector3D hitPoint;
    hitPoint.sumScaled(camera->eyePosition, distance, rayDirection);

    double u;
    double v;
    patch->uv(&hitPoint, &u, &v);
    RadianceImageExporter::clipUv(patch->numberOfVertices, &u, &v);

    Vector3D eyeDirection(-rayDirection.x, -rayDirection.y, -rayDirection.z);
    return ColorRgb(radianceMethod->getRadiance(camera, patch, u, v, eyeDirection, renderOptions));
}
