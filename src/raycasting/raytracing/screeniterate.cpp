#include <ctime>

#include "common/color.h"
#include "scene/Camera.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/opengl.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/raytracing/screeniterate.h"

// Different functions need to sync with the timer
#define WAKE_UP_RENDER ((unsigned char)1<<1)

class ScreenIterateState {
  public:
    clock_t lastTime;
    unsigned char wakeUp;
};

static ScreenIterateState iState;

/* for counting how much CPU time was used for the computations */
static void
screenIterateUpdateCpuSecs() {
    GLOBAL_raytracer_totalTime += (double)clock() - (double)iState.lastTime;
    iState.lastTime = clock();
}

// ScreenIterateInit : initialise statistics and timers
void
ScreenIterateInit() {
#ifndef NO_EVENT_TIMER
    iState.wakeUp = 0;
#endif

    // Initialize for statistics etc.
    iState.lastTime = clock();
    GLOBAL_raytracer_totalTime = 0.0;
    GLOBAL_raytracer_rayCount = GLOBAL_raytracer_pixelCount = 0;
}

void
ScreenIterateFinish() {
    screenIterateUpdateCpuSecs();
}

void
ScreenIterateSequential(SCREEN_ITERATE_CALLBACK callback, void *data) {
    int i, j, width, height;
    COLOR col;
    RGB *rgb;

    ScreenIterateInit();

    width = GLOBAL_camera_mainCamera.xSize;
    height = GLOBAL_camera_mainCamera.ySize;
    rgb = new RGB[width];

    // Shoot rays through all the pixels
    for ( j = 0; j < height; j++ ) {
        for ( i = 0; i < width; i++ ) {
            col = callback(i, j, data);
            radianceToRgb(col, &rgb[i]);
            GLOBAL_raytracer_pixelCount++;
        }

        openGlRenderPixels(0, height - j - 1, width, 1, rgb);
    }

    delete[] rgb;

    ScreenIterateFinish();
}


/*******************************************************************/
/* Some utility routines for progressive tracing */
static inline void
FillRect(int x0, int y0, int x1, int y1, RGB col, RGB *rgb) {
    int x, y;

    for ( x = x0; x < x1; x++ ) {
        for ( y = y0; y < y1; y++ ) {
            rgb[y * GLOBAL_camera_mainCamera.xSize + x] = col;
        }
    }
}


void
ScreenIterateProgressive(SCREEN_ITERATE_CALLBACK callback, void *data) {
    int i;
    int width;
    int height;
    COLOR col;
    RGB pixelRGB{};
    RGB *rgb;
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

    width = GLOBAL_camera_mainCamera.xSize;
    height = GLOBAL_camera_mainCamera.ySize;
    rgb = new RGB[width * height]; // We need a full screen!

    for ( i = 0; i < width * height; i++ ) {
        rgb[i] = GLOBAL_material_black;
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

            yMin = intMin(y0, yMin);
            yMax = intMax(y1, yMax);

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
                    col = callback(x0, height - y0 - 1, data);
                    radianceToRgb(col, &pixelRGB);
                    FillRect(x0, y0, x1, y1, pixelRGB, rgb);

                    GLOBAL_raytracer_pixelCount++;

                    if ( iState.wakeUp & WAKE_UP_RENDER) {
                        iState.wakeUp &= ~WAKE_UP_RENDER;
                        if ( (yMax > 0) && (yMax > yMin) ) {
                            openGlRenderPixels(0, yMin, width, yMax - yMin, rgb + yMin * width);
                        }
                        yMin = intMax(0, yMax - stepSize);
                    }
                }

                x0 = x1;
                xSteps++;
            }

            if ( yMax >= height ) {
                if ( (yMax > yMin) ) {
                    openGlRenderPixels(0, yMin, width, yMax - yMin,
                                       rgb + yMin * width);
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
