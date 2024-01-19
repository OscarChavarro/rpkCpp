/**
Original version by Vincent Masselus adapted by Pieter Peers (2001-06-01)
*/

#include <ctime>

#include "raycasting/common/raytools.h"
#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/simple/RayMatterOptions.h"
#include "raycasting/simple/RayMatter.h"

RayMatter::RayMatter(ScreenBuffer *screen) {
    if ( screen == nullptr ) {
        scrn = new ScreenBuffer(nullptr);
        doDeleteScreen = false;
    } else {
        scrn = screen;
        doDeleteScreen = false;
    }

    Filter = nullptr;
    interrupt_requested = false;
    scrn->SetRGBImage(true);
}

RayMatter::~RayMatter() {
    if ( doDeleteScreen ) {
        delete scrn;
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
    interrupt_requested = false;
    COLOR matte;

    CheckFilter();

    long width = GLOBAL_camera_mainCamera.hres;
    long height = GLOBAL_camera_mainCamera.vres;

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
                ray.pos = GLOBAL_camera_mainCamera.eyep;
                ray.dir = scrn->GetPixelVector(x, y, xi1, xi2);
                VECTORNORMALIZE(ray.dir);

                // check if hit
                if ( FindRayIntersection(&ray, nullptr, nullptr, nullptr) != nullptr ) {
                    hits++;
                }
            }

            // add matte value to screenbuffer
            float value = (hits / GLOBAL_rayCasting_rayMatterState.samplesPerPixel);
            if ( value > 1.0 ) {
                value = 1.0;
            }

            colorSet(matte, value, value, value);
            scrn->Add(x, y, matte);
        }

        scrn->RenderScanline(y);
        if ( interrupt_requested ) {
            break;
        }
    }

    GLOBAL_raytracer_totalTime = (float) (clock() - t) / (float) CLOCKS_PER_SEC;
    GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
}

void
RayMatter::display() {
    scrn->Render();
}

void
RayMatter::save(ImageOutputHandle *ip) {
    scrn->WriteFile(ip);
}

void
RayMatter::interrupt() {
    interrupt_requested = true;
}

static RayMatter *rm = nullptr;
RayMattingState GLOBAL_rayCasting_rayMatterState;

static void IRayMatte(ImageOutputHandle *ip) {
    if ( rm ) {
        delete rm;
    }
    rm = new RayMatter(nullptr);
    rm->Matting();
    if ( ip ) {
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
Interrupt() {
    if ( rm ) {
        rm->interrupt();
    }
}

static void
Terminate() {
    if ( rm ) {
        delete rm;
    }
    rm = nullptr;
}

static void
Initialize() {
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
    Interrupt,
    Terminate
};
