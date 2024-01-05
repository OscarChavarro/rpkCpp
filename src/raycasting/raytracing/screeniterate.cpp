#include <ctime>

#include "material/color.h"
#include "shared/camera.h"
#include "shared/render.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "raycasting/common/raytools.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/raytracing/screeniterate.h"

/* Different functions need to sync with the timer */
#define WAKE_UP_RENDER  ((unsigned char)1<<1)

class SCREENITERATESTATE {
  public:
    clock_t lastTime;
    unsigned char wake_up;
};

static SCREENITERATESTATE iState;

/* for counting how much CPU time was used for the computations */
static void
UpdateCpuSecs() {
    GLOBAL_Raytracer_totalTime += (double)clock() - (double)iState.lastTime;
    iState.lastTime = clock();
}

// ScreenIterateInit : initialise statistics and timers
void
ScreenIterateInit() {
    interrupt_raytracing = false;

#ifndef NO_EVENT_TIMER
    iState.wake_up = 0;
#endif

    /* initialize for statistics etc... */
    iState.lastTime = clock();
    GLOBAL_Raytracer_totalTime = 0.;
    Global_Raytracer_rayCount = Global_Raytracer_pixelCount = 0;
}

void
ScreenIterateFinish() {
    UpdateCpuSecs();
}

void
ScreenIterateSequential(SCREENITERATECALLBACK callback, void *data) {
    int i, j, width, height;
    COLOR col;
    RGB *rgb;

    ScreenIterateInit();

    width = GLOBAL_camera_mainCamera.hres;
    height = GLOBAL_camera_mainCamera.vres;
    rgb = new RGB[width];

    /* shoot rays through all the pixels */

    for ( j = 0; j < height && !interrupt_raytracing; j++ ) {
        for ( i = 0; i < width && !interrupt_raytracing; i++ ) {
            col = callback(i, j, data);
            RadianceToRGB(col, &rgb[i]);
            Global_Raytracer_pixelCount++;
        }

        RenderPixels(0, height - j - 1, width, 1, rgb);
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
            rgb[y * GLOBAL_camera_mainCamera.hres + x] = col;
        }
    }
}


void
ScreenIterateProgressive(SCREENITERATECALLBACK callback, void *data) {
    int i, width, height;
    COLOR col;
    RGB pixelRGB;
    RGB *rgb;
    int x0, y0, x1, y1, stepsize, xsteps, ysteps;
    int xstep_done, ystep_done, skip;
    int ymin, ymax;

    ScreenIterateInit();

    width = GLOBAL_camera_mainCamera.hres;
    height = GLOBAL_camera_mainCamera.vres;
    rgb = new RGB[width * height]; // We need a full screen !

    for ( i = 0; i < width * height; i++ ) {
        rgb[i] = GLOBAL_material_black;
    }

    stepsize = 64;
    skip = false;  // First iteration all squares need to be filled
    ymin = height + 1;
    ymax = -1;

    while ((stepsize > 0) && (!interrupt_raytracing)) {
        y0 = 0;
        ysteps = 0;
        ystep_done = false;

        while ( !ystep_done && (!interrupt_raytracing)) {
            y1 = y0 + stepsize;
            if ( y1 >= height ) {
                y1 = height;
                ystep_done = true;
            }

            ymin = MIN(y0, ymin);
            ymax = MAX(y1, ymax);

            x0 = 0;
            xsteps = 0;
            xstep_done = false;

            while ( !xstep_done && (!interrupt_raytracing)) {
                x1 = x0 + stepsize;

                if ( x1 >= width ) {
                    x1 = width;
                    xstep_done = true;
                }

                if ( !skip || (ysteps & 1) || (xsteps & 1)) {
                    col = callback(x0, height - y0 - 1, data);
                    RadianceToRGB(col, &pixelRGB);
                    FillRect(x0, y0, x1, y1, pixelRGB, rgb);

                    Global_Raytracer_pixelCount++;

                    if ( iState.wake_up & WAKE_UP_RENDER) {
                        iState.wake_up &= ~WAKE_UP_RENDER;
                        if ((ymax > 0) && (ymax > ymin)) {
                            RenderPixels(0, ymin, width, ymax - ymin,
                                         rgb + ymin * width);
                        }

                        ymin = MAX(0, ymax - stepsize);
                        ymax = ymax;
                    }
                }

                x0 = x1;
                xsteps++;
            }

            if ( ymax >= height ) {
                if ((ymax > ymin)) {
                    RenderPixels(0, ymin, width, ymax - ymin,
                                 rgb + ymin * width);
                }
                ymax = -1;
            }

            y0 = y1;
            ysteps++;
        }

        skip = true;
        stepsize /= 2;

    }

    delete[] rgb;

    ScreenIterateFinish();
}
