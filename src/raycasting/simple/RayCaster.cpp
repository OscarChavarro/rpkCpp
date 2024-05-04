/**
Ray casting using the SGL library for rendering Patch pointers into
a software frame buffer directly.
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
    #include <ctime>
#endif

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "IMAGE/imagec.h"
#include "common/Statistics.h"
#include "render/SoftIdsWrapper.h"
#include "raycasting/simple/RayCaster.h"

RayCaster::RayCaster(ScreenBuffer *inScreen, Camera *defaultCamera) {
    if ( inScreen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr, defaultCamera);
        doDeleteScreen = true;
    } else {
        screenBuffer = inScreen;
        doDeleteScreen = true;
    }
}

RayCaster::~RayCaster() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
}

void
RayCaster::clipUv(int numberOfVertices, double *u, double *v) {
    if ( *u > 1.0 - EPSILON ) {
        *u = 1.0 - EPSILON;
    }
    if ( *v > 1.0 - EPSILON ) {
        *v = 1.0 - EPSILON;
    }
    if ( numberOfVertices == 3 && (*u + *v) > 1.0 - EPSILON ) {
        if ( *u > *v ) {
            *u = 1.0 - *v - EPSILON;
        } else {
            *v = 1.0 - *u - EPSILON;
        }
    }
    if ( *u < EPSILON ) {
        *u = EPSILON;
    }
    if ( *v < EPSILON ) {
        *v = EPSILON;
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
    ColorRgb rad{};
    rad.clear();

    if ( radianceMethod != nullptr ) {
        // Ray pointing from the eye through the center of the pixel.
        Ray ray;
        ray.pos = camera->eyePosition;
        ray.dir = screenBuffer->getPixelVector(x, y);
        vectorNormalize(ray.dir);

        // Find intersection point of ray with patch
        Vector3D point;
        float dist = vectorDotProduct(patch->normal, ray.dir);
        dist = -(vectorDotProduct(patch->normal, ray.pos) + patch->planeConstant) / dist;
        vectorSumScaled(ray.pos, dist, ray.dir, point);

        // Find surface coordinates of hit point on patch
        double u;
        double v;
        patch->uv(&point, &u, &v);

        // Boundary check is necessary because Z-buffer algorithm does
        // not yield exactly the same result as ray tracing at patch
        // boundaries.
        clipUv(patch->numberOfVertices, &u, &v);

        // Reverse ray direction and get radiance emitted at hit point towards the eye
        Vector3D dir(-ray.dir.x, -ray.dir.y, -ray.dir.z);
        rad = radianceMethod->getRadiance(camera, patch, u, v, dir, renderOptions);
    }
    return rad;
}

void
RayCaster::render(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    #ifdef RAYTRACING_ENABLED
        clock_t t = clock();
    #endif

    SoftIdsWrapper *idRenderer = new SoftIdsWrapper(scene, renderOptions);

    long width;
    long height;
    idRenderer->getSize(&width, &height);
    if ( width != screenBuffer->getHRes() || height != screenBuffer->getVRes() ) {
        logFatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
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
        GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
        GLOBAL_raytracer_rayCount = 0;
        GLOBAL_raytracer_pixelCount = 0;
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

#ifdef RAYTRACING_ENABLED
static RayCaster *globalRayCaster = nullptr;

/**
Returns false if there is no previous image and true if there is
*/
static int
rayCasterRedisplay() {
    if ( !globalRayCaster ) {
        return false;
    }

    globalRayCaster->display();
    return true;
}

static int
rayCasterSaveImage(ImageOutputHandle *ip) {
    if ( !globalRayCaster ) {
        return false;
    }

    globalRayCaster->save(ip);
    return true;
}

static void
rayCasterTerminate() {
    if ( globalRayCaster ) {
        delete globalRayCaster;
    }
    globalRayCaster = nullptr;
}

static void
rayCasterDefaults() {
}

static void
rayCasterParseHWRCastOptions(int * /*argc*/, char ** /*argv*/) {
}

static void
rayCasterInitialize(java::ArrayList<Patch *> * /*lightPatches*/) {
}

static void
rayCasterExecute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions) {
    if ( globalRayCaster != nullptr ) {
        delete globalRayCaster;
    }
    globalRayCaster = new RayCaster(nullptr, scene->camera);
    globalRayCaster->render(scene, radianceMethod, renderOptions);
    if ( globalRayCaster != nullptr && ip != nullptr ) {
        globalRayCaster->save(ip);
    }
}
#endif

/**
Ray-Casts the current Radiance solution. Output is displayed on the screen
and saved into the file with given name and file pointer. 'isPipe'
reflects whether this file pointer is a pipe or not.
*/
void
rayCast(
    const char *fileName,
    FILE *fp,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) {
    ImageOutputHandle *img = nullptr;

    if ( fp ) {
        img = createRadianceImageOutputHandle(
            fileName,
            fp,
            isPipe,
            scene->camera->xSize,
            scene->camera->ySize,
            (float)GLOBAL_statistics.referenceLuminance / 179.0f);

        if ( img == nullptr ) {
            return;
        }
    }

    RayCaster *rc = new RayCaster(nullptr, scene->camera);
    rc->render(scene, radianceMethod, renderOptions);
    if ( img != nullptr ) {
        rc->save(img);
    }
    delete rc;

    if ( img ) {
        deleteImageOutputHandle(img);
    }
}

#ifdef RAYTRACING_ENABLED
Raytracer GLOBAL_rayCasting_RayCasting = {
    "RayCasting",
    4,
    "Ray Casting",
    rayCasterDefaults,
    rayCasterParseHWRCastOptions,
    rayCasterInitialize,
    rayCasterExecute,
    rayCasterRedisplay,
    rayCasterSaveImage,
    rayCasterTerminate
};
#endif
