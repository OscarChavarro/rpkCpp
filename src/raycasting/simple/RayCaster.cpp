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
#include "material/statistics.h"
#include "render/SoftIdsWrapper.h"
#include "raycasting/simple/RayCaster.h"

RayCaster::RayCaster(ScreenBuffer *inScreen) {
    if ( inScreen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr);
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
RayCaster::getRadianceAtPixel(int x, int y, Patch *patch, RadianceMethod *context) {
    ColorRgb rad{};
    colorClear(rad);

    if ( context != nullptr ) {
        // Ray pointing from the eye through the center of the pixel.
        Ray ray;
        ray.pos = GLOBAL_camera_mainCamera.eyePosition;
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
        rad = context->getRadiance(patch, u, v, dir);
    }
    return rad;
}

void
RayCaster::render(java::ArrayList<Patch *> *scenePatches, RadianceMethod *context) {
    #ifdef RAYTRACING_ENABLED
        clock_t t = clock();
    #endif

    Soft_ID_Renderer *idRenderer = new Soft_ID_Renderer(scenePatches);

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
                ColorRgb rad = getRadianceAtPixel(x, y, patch, context);
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
rayCasterExecute(ImageOutputHandle *ip, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> * /*lightPatches*/, RadianceMethod *context) {
    if ( globalRayCaster != nullptr ) {
        delete globalRayCaster;
    }
    globalRayCaster = new RayCaster(nullptr);
    globalRayCaster->render(scenePatches, context);
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
rayCast(char *fileName, FILE *fp, int isPipe, RadianceMethod *context) {
    ImageOutputHandle *img = nullptr;

    if ( fp ) {
        img = createRadianceImageOutputHandle(
            fileName,
            fp,
            isPipe,
            GLOBAL_camera_mainCamera.xSize,
            GLOBAL_camera_mainCamera.ySize,
            (float)GLOBAL_statistics.referenceLuminance / 179.0f);

        if ( img == nullptr ) {
            return;
        }
    }

    RayCaster *rc = new RayCaster(nullptr);
    rc->render(nullptr, context);
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
