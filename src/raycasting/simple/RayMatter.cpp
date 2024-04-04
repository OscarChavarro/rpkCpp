/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/

#include <ctime>

#include "raycasting/common/raytools.h"
#include "render/ScreenBuffer.h"
#include "raycasting/simple/RayMatterOptions.h"
#include "raycasting/simple/RayMatter.h"

RayMatter::RayMatter(ScreenBuffer *screen) {
    if ( screen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr);
        doDeleteScreen = false;
    } else {
        screenBuffer = screen;
        doDeleteScreen = false;
    }

    Filter = nullptr;
    screenBuffer->setRgbImage(true);
}

RayMatter::~RayMatter() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
    if ( Filter != nullptr ) {
        delete Filter;
    }
}

void
RayMatter::CheckFilter() {
    if ( Filter ) {
        delete Filter;
    }

    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_BOX_FILTER ) {
        Filter = new BoxFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_TENT_FILTER ) {
        Filter = new TentFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_GAUSS_FILTER ) {
        Filter = new NormalFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_GAUSS2_FILTER ) {
        Filter = new NormalFilter(.5, 1.5);
    }
}

void
RayMatter::Matting() {
    clock_t t = clock();
    ColorRgb matte;

    CheckFilter();

    long width = GLOBAL_camera_mainCamera.xSize;
    long height = GLOBAL_camera_mainCamera.ySize;

    // TODO SITHMASTER: Main paralelizable loop for ray matter
    for ( long y = 0; y < height; y++ ) {
        for ( long x = 0; x < width; x++ ) {
            float hits = 0;

            for ( int i = 0; i < GLOBAL_rayCasting_rayMatterState.samplesPerPixel; i++ ) {
                // uniform random var
                double xi1 = drand48();
                double xi2 = drand48();

                // insert non-uniform sampling here
                Filter->sample(&xi1, &xi2);

                // generate ray
                Ray ray;
                ray.pos = GLOBAL_camera_mainCamera.eyePosition;
                ray.dir = screenBuffer->getPixelVector((int)x, (int)y, (float)xi1, (float)xi2);
                vectorNormalize(ray.dir);

                // check if hit
                if ( findRayIntersection(&ray, nullptr, nullptr, nullptr) != nullptr ) {
                    hits++;
                }
            }

            // Add matte value to screen buffer
            float value = (hits / (float)GLOBAL_rayCasting_rayMatterState.samplesPerPixel);
            if ( value > 1.0 ) {
                value = 1.0;
            }

            matte.set(value, value, value);
            screenBuffer->add((int)x, (int)y, matte);
        }

        screenBuffer->renderScanline((int)y);
    }

    GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
    GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
}

void
RayMatter::display() {
    screenBuffer->render();
}

void
RayMatter::save(ImageOutputHandle *ip) {
    screenBuffer->writeFile(ip);
}

static RayMatter *rm = nullptr;
RayMattingState GLOBAL_rayCasting_rayMatterState;

static void
IRayMatte(ImageOutputHandle *ip, java::ArrayList<Patch *> * /*scenePatches*/, java::ArrayList<Patch *> * /*lightPatches*/, RadianceMethod *context) {
    if ( rm ) {
        delete rm;
    }
    rm = new RayMatter(nullptr);
    rm->Matting();
    if ( ip && rm != nullptr ) {
        rm->save(ip);
    }
}

/**
Returns false if there is no previous image and true if there is
*/
static int
Redisplay() {
    if ( !rm ) {
        return false;
    }

    rm->display();
    return true;
}

static int
SaveImage(ImageOutputHandle *ip) {
    if ( !rm ) {
        return false;
    }

    rm->save(ip);
    return true;
}

static void
Terminate() {
    if ( rm ) {
        delete rm;
    }
    rm = nullptr;
}

static void
Initialize(java::ArrayList<Patch *> * /*lightPatches*/) {
}

Raytracer GLOBAL_rayCasting_RayMatting = {
    "RayMatting",
    4,
    "Ray Matting",
    rayMattingDefaults,
    rayMattingParseOptions,
    Initialize,
    IRayMatte,
    Redisplay,
    SaveImage,
    Terminate
};
