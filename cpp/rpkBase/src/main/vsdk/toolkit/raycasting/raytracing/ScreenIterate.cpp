#include "java/lang/Math.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/render/Softids.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterateState.h"
#include "vsdk/toolkit/raycasting/raytracing/ScreenIterate.h"

unsigned char
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

/**
Some utility routines for progressive tracing
*/
void
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
