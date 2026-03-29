#include "java/lang/System.h"
#include "tonemap/ToneMap.h"
#include "render/Opengl.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/raytracing/ScreenIterateState.h"
#include "raycasting/raytracing/ScreenIterate.h"

static inline unsigned char
wakeUpRender() {
    return static_cast<unsigned char>(1u << 1);
}

static ScreenIterateState iState;

/**
For counting how much CPU time was used for the computations
*/
static void
screenIterateUpdateCpuSecs() {
    const long long now = java::lang::System::nanoTime();
    GLOBAL_raytracer_totalTime += static_cast<double>(now - iState.lastTime) / 1000000000.0;
    iState.lastTime = now;
}

// ScreenIterateInit : initialise statistics and timers
void
ScreenIterateInit() {
#ifndef NO_EVENT_TIMER
    iState.wakeUp = 0;
#endif

    // initialize for statistics etc.
    iState.lastTime = java::lang::System::nanoTime();
    GLOBAL_raytracer_totalTime = 0.0;
    GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
}

void
ScreenIterateFinish() {
    screenIterateUpdateCpuSecs();
}

void
screenIterateSequential(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback, void *data)
{
    int width;
    int height;
    ColorRgb col;
    ColorRgb *rgb;

    ScreenIterateInit();

    width = camera->xSize;
    height = camera->ySize;
    rgb = new ColorRgb[width];

    // Shoot rays through all the pixels
    for ( int i = 0; i < height; i++ ) {
        for ( int j = 0; j < width; j++ ) {
            col = callback(camera, sceneVoxelGrid, sceneBackground, j, i, data);
            ToneMap::radianceToRgb(col, &rgb[j]);
            GLOBAL_raytracer_pixelCount++;
        }

        softRenderPixels(width, 1, rgb);
    }

    delete[] rgb;

    ScreenIterateFinish();
}

/**
Some utility routines for progressive tracing
*/
static inline void
fillRect(
    const Camera *camera,
    int x0,
    int y0,
    int x1,
    int y1,
    ColorRgb col,
    ColorRgb *rgb)
{
    for ( int x = x0; x < x1; x++ ) {
        for ( int y = y0; y < y1; y++ ) {
            rgb[y * camera->xSize + x] = col;
        }
    }
}

void
screenIterateProgressive(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback,
    void *data)
{
    int width;
    int height;
    ColorRgb col;
    ColorRgb pixelRGB{};
    ColorRgb *rgb;
    int x0;
    int y0;
    int x1;
    int y1;
    int stepSize;
    int xSteps;
    int ySteps;
    int xStepDone;
    int yStepDone;
    int skip;
    int yMin;
    int yMax;

    ScreenIterateInit();

    width = camera->xSize;
    height = camera->ySize;
    rgb = new ColorRgb[width * height]; // We need a full screen!

    ColorRgb white = {1.0, 1.0, 1.0};

    for ( int i = 0; i < width * height; i++ ) {
        rgb[i] = white;
    }

    stepSize = 64;
    skip = false;  // First iteration all squares need to be filled
    yMin = height + 1;
    yMax = -1;

    while ( stepSize > 0 ) {
        y0 = 0;
        ySteps = 0;
        yStepDone = false;

        while ( !yStepDone ) {
            y1 = y0 + stepSize;
            if ( y1 >= height ) {
                y1 = height;
                yStepDone = true;
            }

            yMin = java::Math::min(y0, yMin);
            yMax = java::Math::max(y1, yMax);

            x0 = 0;
            xSteps = 0;
            xStepDone = false;

            while ( !xStepDone ) {
                x1 = x0 + stepSize;

                if ( x1 >= width ) {
                    x1 = width;
                    xStepDone = true;
                }

                if ( !skip || (ySteps & 1) || (xSteps & 1) ) {
                    col = callback(camera, sceneVoxelGrid, sceneBackground, x0, height - y0 - 1, data);
                    ToneMap::radianceToRgb(col, &pixelRGB);
                    fillRect(camera, x0, y0, x1, y1, pixelRGB, rgb);

                    GLOBAL_raytracer_pixelCount++;

                    if ( iState.wakeUp & wakeUpRender() ) {
                        iState.wakeUp &= static_cast<unsigned char>(~wakeUpRender());
                        if ( (yMax > 0) && (yMax > yMin) ) {
                            softRenderPixels(width, yMax - yMin, rgb + yMin * width);
                        }
                        yMin = java::Math::max(0, yMax - stepSize);
                    }
                }

                x0 = x1;
                xSteps++;
            }

            if ( yMax >= height ) {
                if ( yMax > yMin ) {
                    softRenderPixels(width, yMax - yMin, rgb + yMin * width);
                }
                yMax = -1;
            }

            y0 = y1;
            ySteps++;
        }

        skip = true;
        stepSize /= 2;

    }

    delete[] rgb;

    ScreenIterateFinish();
}
