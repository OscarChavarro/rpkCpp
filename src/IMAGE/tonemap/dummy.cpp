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

static COLOR dummyScaleForComputations(COLOR radiance) {
    return radiance;
}

static COLOR dummyScaleForDisplay(COLOR radiance) {
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
