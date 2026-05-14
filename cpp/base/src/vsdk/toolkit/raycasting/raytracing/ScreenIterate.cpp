#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/render/Softids.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterateState.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterate.h"

inline unsigned char
ScreenIterate::wakeUpRender() {
    return static_cast<unsigned char>(1u << 1);
}

ScreenIterateState ScreenIterate::state;

/**
For counting how much CPU time was used for the computations
*/
void
ScreenIterate::updateCpuSecs() {
    const long long now = java::System::nanoTime();
    Statistics::instance().rayTracer.totalTime += static_cast<double>(now - state.lastTime) / 1000000000.0;
    state.lastTime = now;
}

// ScreenIterateInit : initialise statistics and timers
void
ScreenIterate::init() {
#ifndef NO_EVENT_TIMER
    state.wakeUp = 0;
#endif

    // initialize for statistics etc.
    state.lastTime = java::System::nanoTime();
    Statistics::instance().rayTracer.resetCounters();
}

void
ScreenIterate::finish() {
    updateCpuSecs();
}

void
ScreenIterate::sequential(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback,
    void *data,
    const ToneMappingContext &toneMapOptions)
{
    int width;
    int height;
    ColorRgbMutable col(0.0, 0.0, 0.0);
    ColorRgbMutable *rgb;

    init();

    width = camera->xSize;
    height = camera->ySize;
    rgb = new ColorRgbMutable[width];

    // Shoot rays through all the pixels
    for ( int i = 0; i < height; i++ ) {
        for ( int j = 0; j < width; j++ ) {
            col = callback(camera, sceneVoxelGrid, sceneBackground, j, i, data);
            ToneMap::radianceToRgb(col, &rgb[j], toneMapOptions);
            Statistics::instance().rayTracer.pixelCount++;
        }

        SoftIds::softRenderPixels(width, 1, rgb, toneMapOptions);
    }

    delete[] rgb;

    finish();
}

/**
Some utility routines for progressive tracing
*/
inline void
ScreenIterate::fillRect(
    const Camera *camera,
    int x0,
    int y0,
    int x1,
    int y1,
    ColorRgbMutable col,
    ColorRgbMutable *rgb)
{
    for ( int x = x0; x < x1; x++ ) {
        for ( int y = y0; y < y1; y++ ) {
            rgb[y * camera->xSize + x] = col;
        }
    }
}

void
ScreenIterate::progressive(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SCREEN_ITERATE_CALLBACK callback,
    void *data,
    const ToneMappingContext &toneMapOptions)
{
    int width;
    int height;
    ColorRgbMutable col(0.0, 0.0, 0.0);
    ColorRgbMutable pixelRGB{};
    ColorRgbMutable *rgb;
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

    init();

    width = camera->xSize;
    height = camera->ySize;
    rgb = new ColorRgbMutable[width * height]; // We need a full screen!

    ColorRgbMutable white = {1.0, 1.0, 1.0};

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
                    ToneMap::radianceToRgb(col, &pixelRGB, toneMapOptions);
                    fillRect(camera, x0, y0, x1, y1, pixelRGB, rgb);

                    Statistics::instance().rayTracer.pixelCount++;

                    if ( state.wakeUp & wakeUpRender() ) {
                        state.wakeUp &= static_cast<unsigned char>(~wakeUpRender());
                        if ( (yMax > 0) && (yMax > yMin) ) {
                            SoftIds::softRenderPixels(width, yMax - yMin, rgb + yMin * width, toneMapOptions);
                        }
                        yMin = java::Math::max(0, yMax - stepSize);
                    }
                }

                x0 = x1;
                xSteps++;
            }

            if ( yMax >= height ) {
                if ( yMax > yMin ) {
                    SoftIds::softRenderPixels(width, yMax - yMin, rgb + yMin * width, toneMapOptions);
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

    finish();
}
