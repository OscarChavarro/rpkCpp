/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/render/SoftIdsWrapper.h"
#include "vsdk/toolkit/raycasting/simple/RayCaster.h"

const char *const RayCaster::NAME = "Ray Casting";
RayCaster *RayCaster::rayCaster = NULL;

RayCaster::RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera, ToneMappingContext *toneMapOptions) {
    if ( inScreen == NULL ) {
        screenBuffer = new ScreenBuffer(NULL, defaultCamera, toneMapOptions);
        doDeleteScreen = true;
    } else {
        screenBuffer = inScreen;
        screenBuffer->setToneMappingContext(toneMapOptions);
        doDeleteScreen = true;
    }
}

RayCaster::~RayCaster() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
}

void
RayCaster::defaults() {
}

const char *
RayCaster::getName() const {
    return NAME;
}

void
RayCaster::initialize(const ArrayList<Patch *> */*lightPatches*/) const {
}

void
RayCaster::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions) const
{
    if ( rayCaster != NULL ) {
        delete rayCaster;
    }
    rayCaster = new RayCaster(NULL, scene->camera, toneMapOptions);
    rayCaster->render(scene, radianceMethod, toneMapOptions, renderOptions);
    if ( rayCaster != NULL && ip != NULL ) {
        rayCaster->save(ip);
    }
}

bool
RayCaster::saveImage(ImageOutputHandle *imageOutputHandle) const {
    if ( !rayCaster ) {
        return false;
    }

    rayCaster->save(imageOutputHandle);
    return true;
}

void
RayCaster::terminate() const {
    if ( rayCaster ) {
        delete rayCaster;
    }
    rayCaster = NULL;
}

void
RayCaster::clipUv(int numberOfVertices, double *u, double *v) {
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

/**
Determines the radiance of the nearest patch visible through the pixel
(x,y). P shall be the nearest patch visible in the pixel.
*/
inline ColorRgb
RayCaster::getRadianceAtPixel(
    Camera *camera,
    int x,
    int y,
    Patch *patch,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) const
{
    ColorRgb radiance = ColorRgb();
    radiance = ColorRgb(0.0, 0.0, 0.0);

    if ( radianceMethod != NULL ) {
        // Ray pointing from the eye through the center of the pixel.
        Ray ray;
        ray.setOrigin(camera->eyePosition);
        ray.setDirection(screenBuffer->getPixelVector(x, y));
        ray.setDirection(ray.getDirection().normalized(Numeric::EPSILON));

        // Find intersection point of ray with patch
        Vector3Dd point;
        double dist = patch->normal.dotProduct(ray.getDirection());
        dist = -(patch->normal.dotProduct(ray.getOrigin()) + patch->planeConstant) / dist;
        point = ray.getOrigin().add(ray.getDirection().multiply(((double)(dist))));

        // Find surface coordinates of hit point on patch
        double u;
        double v;
        patch->uv(&point, &u, &v);

        // Boundary check is necessary because Z-buffer algorithm does
        // not yield exactly the same result as ray tracing at patch
        // boundaries.
        clipUv(patch->numberOfVertices, &u, &v);

        // Reverse ray direction and get radiance emitted at hit point towards the eye
        Vector3Dd dir(-ray.getDirection().x(), -ray.getDirection().y(), -ray.getDirection().z());
        radiance = ColorRgb(radianceMethod->getRadiance(camera, patch, u, v, dir, renderOptions));
    }
    return radiance;
}

void
RayCaster::render(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions)
{
    screenBuffer->setToneMappingContext(toneMapOptions);
#ifdef RAYTRACING_ENABLED
    long long t = System::nanoTime();
#endif

    SoftIdsWrapper *idRenderer = new SoftIdsWrapper(scene, renderOptions);

    long width;
    long height;
    idRenderer->getSize(&width, &height);
    if ( width != screenBuffer->getHRes() || height != screenBuffer->getVRes() ) {
        Logger::reportMessage("RayCaster", Logger::FATAL_ERROR, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // This is the main loop for ray-casting
    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer->getPatchAtPixel(x, y);
            if ( patch != NULL ) {
                ColorRgb rad = getRadianceAtPixel(scene->camera, x, y, patch, radianceMethod, renderOptions);
                screenBuffer->add(x, y, rad);
            }
        }

        screenBuffer->renderScanline(y);
    }

    delete idRenderer;

#ifdef RAYTRACING_ENABLED
    Statistics::instance().rayTracer.totalTime = ((double)(System::nanoTime() - t)) / 1000000000.0;
    Statistics::instance().rayTracer.rayCount = 0;
    Statistics::instance().rayTracer.pixelCount = 0;
#endif
}

void
RayCaster::display() {
    screenBuffer->render();
}

void
RayCaster::save(ImageOutputHandle *ip) {
    screenBuffer->writeFile(ip);
}
