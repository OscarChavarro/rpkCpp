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

void
RayCaster::render(GETRADIANCE_FT getRadiance = nullptr, java::ArrayList<Patch *> *scenePatches = nullptr) {
    #ifdef RAYTRACING_ENABLED
        clock_t t = clock();
    #endif

    if ( getRadiance == nullptr ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
            getRadiance = GLOBAL_radiance_currentRadianceMethodHandle->GetRadiance;
        }
    }

    long width;
    long height;
    long x;
    long y;

    Soft_ID_Renderer *id_renderer = new Soft_ID_Renderer(scenePatches);
    id_renderer->get_size(&width, &height);
    if ( width != screenBuffer->getHRes() || height != screenBuffer->getVRes() ) {
        logFatal(-1, "RayCaster::render", "ID buffer size doesn't match screen size");
    }

    // TODO SITHMASTER: This is the main paralelizable loop for ray-casting
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            Patch *patch = id_renderer->getPatchAtPixel(x, y);
            COLOR rad = getRadianceAtPixel(x, y, patch, getRadiance);
            screenBuffer->add(x, y, rad);
        }

        screenBuffer->renderScanline(y);
    }

    delete id_renderer;

    #ifdef RAYTRACING_ENABLED
        GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
        GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
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
rayCasterInterrupt() {
}

static void
rayCasterTerminate() {
    if ( globalRayCaster ) {
        delete globalRayCaster;
    }
    globalRayCaster = 0;
}

static void
rayCasterDefaults() {
}

static void
rayCasterParseHWRCastOptions(int *argc, char **argv) {
}

static void
rayCasterInitialize(java::ArrayList<Patch *> * /*lightPatches*/) {
}

static void
rayCasterExecute(ImageOutputHandle *ip, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    if ( globalRayCaster ) {
        delete globalRayCaster;
    }
    globalRayCaster = new RayCaster();
    globalRayCaster->render(nullptr, scenePatches);
    if ( ip ) {
        globalRayCaster->save(ip);
    }
}

#endif

/**
Ray-Casts the current Radiance solution. Output is displayed on the sceen
and saved into the file with given name and file pointer. 'ispipe'
reflects whether this file pointer is a pipe or not.
*/
void
rayCast(char *fileName, FILE *fp, int isPipe) {
    ImageOutputHandle *img = nullptr;

    if ( fp ) {
        img = createRadianceImageOutputHandle(fileName, fp, isPipe,
                                              GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize,
                                              GLOBAL_statistics_referenceLuminance / 179.0);
        if ( !img ) {
            return;
        }
    }

    RayCaster *rc = new RayCaster;
    rc->render();
    if ( img ) {
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
    rayCasterInterrupt,
    rayCasterTerminate
};
#endif
