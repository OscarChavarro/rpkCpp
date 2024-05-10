/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <ctime>

#include "raycasting/common/raytools.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/BoxFilter.h"
#include "raycasting/common/TentFilter.h"
#include "raycasting/common/NormalFilter.h"
#include "raycasting/simple/RayMatterOptions.h"
#include "raycasting/simple/RayMatter.h"

RayMatter::RayMatter(ScreenBuffer *screen, Camera *camera) {
    if ( screen == nullptr ) {
        screenBuffer = new ScreenBuffer(nullptr, camera);
        doDeleteScreen = false;
    } else {
        screenBuffer = screen;
        doDeleteScreen = false;
    }

    pixelFilter = nullptr;
    screenBuffer->setRgbImage(true);
}

RayMatter::~RayMatter() {
    if ( doDeleteScreen ) {
        delete screenBuffer;
    }
    if ( pixelFilter != nullptr ) {
        delete pixelFilter;
    }
}

void
RayMatter::checkFilter() {
    if ( pixelFilter ) {
        delete pixelFilter;
    }

    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_BOX_FILTER ) {
        pixelFilter = new BoxFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_TENT_FILTER ) {
        pixelFilter = new TentFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_GAUSS_FILTER ) {
        pixelFilter = new NormalFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RM_GAUSS2_FILTER ) {
        pixelFilter = new NormalFilter(.5, 1.5);
    }
}

void
RayMatter::doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid) {
    clock_t t = clock();
    ColorRgb matte;

    checkFilter();

    long width = camera->xSize;
    long height = camera->ySize;

    // Main loop for ray matter
    for ( long y = 0; y < height; y++ ) {
        for ( long x = 0; x < width; x++ ) {
            float hits = 0;

            for ( int i = 0; i < GLOBAL_rayCasting_rayMatterState.samplesPerPixel; i++ ) {
                // Uniform random var
                double xi1 = drand48();
                double xi2 = drand48();

                // Insert non-uniform sampling here
                pixelFilter->sample(&xi1, &xi2);

                // Generate ray
                Ray ray;
                ray.pos = camera->eyePosition;
                ray.dir = screenBuffer->getPixelVector((int)x, (int)y, (float)xi1, (float)xi2);
                vectorNormalize(ray.dir);

                // Check if hit
                if ( findRayIntersection(sceneWorldVoxelGrid, &ray, nullptr, nullptr, nullptr) != nullptr ) {
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
iRayMatte(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod * /*radianceMethod*/,
    RenderOptions * /*renderOptions*/)
{
    if ( rm != nullptr ) {
        delete rm;
    }
    rm = new RayMatter(nullptr, scene->camera);
    rm->doMatting(scene->camera, scene->voxelGrid);
    if ( ip && rm != nullptr ) {
        rm->save(ip);
    }
}

/**
Returns false if there is no previous image and true if there is
*/
static int
reDisplay() {
    if ( !rm ) {
        return false;
    }

    rm->display();
    return true;
}

static int
saveImage(ImageOutputHandle *ip) {
    if ( !rm ) {
        return false;
    }

    rm->save(ip);
    return true;
}

static void
terminate() {
    if ( rm ) {
        delete rm;
    }
    rm = nullptr;
}

static void
initialize(java::ArrayList<Patch *> * /*lightPatches*/) {
}

Raytracer GLOBAL_rayCasting_RayMatting = {
    "RayMatting",
    4,
    "Ray Matting",
    rayMattingDefaults,
    rayMattingParseOptions,
    initialize,
    iRayMatte,
    reDisplay,
    saveImage,
    terminate
};

#endif
