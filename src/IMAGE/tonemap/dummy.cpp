/**
Dummy tone map
*/

#include "IMAGE/tonemap/dummy.h"

static void dummyDefaults() {
}

static void dummyInit() {
}

static void dummyTerminate() {
}

static ColorRgb dummyScaleForComputations(ColorRgb radiance) {
    return radiance;
}

static ColorRgb dummyScaleForDisplay(ColorRgb radiance) {
    return radiance;
}

ToneMap GLOBAL_toneMap_dummy = {
    "Dummy",
    "Dummy",
    3,
    dummyDefaults,
    nullptr,
    dummyInit,
    dummyTerminate,
    dummyScaleForComputations,
    dummyScaleForDisplay
};
