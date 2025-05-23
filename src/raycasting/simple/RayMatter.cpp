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
#include "raycasting/simple/RayMatter.h"

static RayMatter *globalRayMatter = nullptr;
RayMatterState GLOBAL_rayCasting_rayMatterState;

char RayMatter::name[12] = "Ray Matting";

RayMatter::RayMatter(ScreenBuffer *screen, const Camera *camera) {
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
RayMatter::defaults() {
    GLOBAL_rayCasting_rayMatterState.filter = TENT_FILTER;
    GLOBAL_rayCasting_rayMatterState.samplesPerPixel = 8;
}

const char *
RayMatter::getName() const {
    return name;
}

void
RayMatter::initialize(const java::ArrayList<Patch *> *lightPatches) const {
}

void
RayMatter::execute(
    ImageOutputHandle *ip,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions) const
{
    if ( globalRayMatter != nullptr ) {
        delete globalRayMatter;
    }
    globalRayMatter = new RayMatter(nullptr, scene->camera);
    globalRayMatter->doMatting(scene->camera, scene->voxelGrid);
    if ( ip && globalRayMatter != nullptr ) {
        globalRayMatter->save(ip);
    }
}

bool
RayMatter::saveImage(ImageOutputHandle *imageOutputHandle) const {
    if ( globalRayMatter == nullptr ) {
        return false;
    }

    globalRayMatter->save(imageOutputHandle);
    return true;
}

void
RayMatter::terminate() const {
    if ( globalRayMatter != nullptr ) {
        delete globalRayMatter;
    }
    globalRayMatter = nullptr;
}

void
RayMatter::createFilter() {
    if ( pixelFilter != nullptr ) {
        delete pixelFilter;
        pixelFilter = nullptr;
    }

    if ( GLOBAL_rayCasting_rayMatterState.filter == RayMatterFilterType::BOX_FILTER ) {
        pixelFilter = new BoxFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RayMatterFilterType::TENT_FILTER ) {
        pixelFilter = new TentFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RayMatterFilterType::GAUSS_FILTER ) {
        pixelFilter = new NormalFilter;
    }
    if ( GLOBAL_rayCasting_rayMatterState.filter == RayMatterFilterType::GAUSS2_FILTER ) {
        pixelFilter = new NormalFilter(0.5, 1.5);
    }
}

void
RayMatter::doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid) {
    clock_t t = clock();

    createFilter();

    // Main loop for ray matter
    for ( int y = 0; y < camera->ySize; y++ ) {
        for ( int x = 0; x < camera->xSize; x++ ) {
            float hits = 0;

            for ( int i = 0; i < GLOBAL_rayCasting_rayMatterState.samplesPerPixel; i++ ) {
                // Uniform random var
                double dx = drand48();
                double dy = drand48();

                // Insert non-uniform sampling here
                if ( pixelFilter != nullptr ) {
                    pixelFilter->sample(&dx, &dy);
                }

                // Generate ray
                Ray ray;
                ray.pos = camera->eyePosition;
                ray.dir = screenBuffer->getPixelVector(x, y, (float)dx, (float)dy);
                ray.dir.normalize(Numeric::EPSILON_FLOAT);

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

            ColorRgb matte;
            matte.set(value, value, value);
            screenBuffer->add(x, y, matte);
        }

        screenBuffer->renderScanline(y);
    }

    GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
    GLOBAL_raytracer_rayCount = 0;
    GLOBAL_raytracer_pixelCount = 0;
}

void
RayMatter::display() {
    screenBuffer->render();
}

void
RayMatter::save(ImageOutputHandle *ip) {
    screenBuffer->writeFile(ip);
}

#endif
