/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/
#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/render/SoftIdsWrapper.h"
#include "vsdk/toolkit/raycasting/simple/RayCaster.h"

const char *const RayCaster::NAME = "Ray Casting";
RayCaster *RayCaster::rayCaster = nullptr;

RayCaster::RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera, ToneMappingContext *toneMapOptions) {
    if ( inScreen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr, defaultCamera, toneMapOptions);
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
RayCaster::initialize(const java::ArrayList<Patch *> */*lightPatches*/) const {
}

void
RayCaster::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RendererConfiguration *renderOptions) const
{
    if ( rayCaster != nullptr ) {
        delete rayCaster;
    }
    rayCaster = new RayCaster(nullptr, scene->camera, toneMapOptions);
    rayCaster->render(scene, radianceMethod, toneMapOptions, renderOptions);
    if ( rayCaster != nullptr && ip != nullptr ) {
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
    rayCaster = nullptr;
}

void
RayCaster::clipUv(int numberOfVertices, double *u, double *v) {
    if ( *u > 1.0 - Numeric::Numeric::EPSILON ) {
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
    const RendererConfiguration *renderOptions) const
{
    ColorRgb radiance{};
    radiance.clear();

    if ( radianceMethod != nullptr ) {
        // Ray pointing from the eye through the center of the pixel.
        Ray ray;
        ray.position = camera->eyePosition;
        ray.direction = screenBuffer->getPixelVector(x, y);
        ray.direction.normalize(Numeric::EPSILON_FLOAT);

        // Find intersection point of ray with patch
        Vector3D point;
        float dist = patch->getNormal().dotProduct(ray.direction);
        dist = -(patch->getNormal().dotProduct(ray.position) + patch->getPlaneConstant()) / dist;
        point.sumScaled(ray.position, dist, ray.direction);

        // Find surface coordinates of hit point on patch
        double u;
        double v;
        patch->uv(&point, &u, &v);

        // Boundary check is necessary because Z-buffer algorithm does
        // not yield exactly the same result as ray tracing at patch
        // boundaries.
        clipUv(patch->getNumberOfVertices(), &u, &v);

        // Reverse ray direction and get radiance emitted at hit point towards the eye
        Vector3D dir(-ray.direction.x, -ray.direction.y, -ray.direction.z);
        radiance = radianceMethod->getRadiance(camera, patch, u, v, dir, renderOptions);
    }
    return radiance;
}

void
RayCaster::render(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    ToneMappingContext *toneMapOptions,
    const RendererConfiguration *renderOptions)
{
    screenBuffer->setToneMappingContext(toneMapOptions);
#ifdef RAYTRACING_ENABLED
    long long t = java::System::nanoTime();
#endif

    SoftIdsWrapper *idRenderer = new SoftIdsWrapper(scene, renderOptions);

    long width;
    long height;
    idRenderer->getSize(&width, &height);
    if ( width != screenBuffer->getHRes() || height != screenBuffer->getVRes() ) {
        Logger::fatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // This is the main loop for ray-casting
    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            Patch *patch = idRenderer->getPatchAtPixel(x, y);
            if ( patch != nullptr ) {
                ColorRgb rad = getRadianceAtPixel(scene->camera, x, y, patch, radianceMethod, renderOptions);
                screenBuffer->add(x, y, rad);
            }
        }

        screenBuffer->renderScanline(y);
    }

    delete idRenderer;

#ifdef RAYTRACING_ENABLED
    Statistics::instance().rayTracer.totalTime = static_cast<double>(java::System::nanoTime() - t) / 1000000000.0;
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
