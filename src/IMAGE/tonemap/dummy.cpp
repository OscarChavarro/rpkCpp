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

static float dummyReverseScaleForComputations(float /*dl*/) {
    return -1.0;
}

TONEMAP GLOBAL_toneMap_dummy = {
    "Dummy",
    "Dummy",
    "dummyButton",
    3,
    dummyDefaults,
    nullptr,
    nullptr,
    dummyInit,
    dummyTerminate,
    dummyScaleForComputations,
    dummyScaleForDisplay,
    dummyReverseScaleForComputations,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};
