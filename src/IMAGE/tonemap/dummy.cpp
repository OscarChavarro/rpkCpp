/**
Dummy tone map
*/

#include "IMAGE/tonemap/dummy.h"

static void Defaults() {
}

static void dummyInit() {
}

static void Terminate() {
}

static COLOR ScaleForComputations(COLOR radiance) {
    return radiance;
}

static COLOR ScaleForDisplay(COLOR radiance) {
    return radiance;
}

static float ReverseScaleForComputations(float dl) {
    return -1.0;
}

TONEMAP TM_Dummy = {
        "Dummy", "Dummy", "dummyButton", 3,
        Defaults,
        (void (*)(int *, char **)) nullptr,
        (void (*)(FILE *)) nullptr,
        dummyInit,
        Terminate,
        ScaleForComputations,
        ScaleForDisplay,
        ReverseScaleForComputations,
        (void (*)(void *)) nullptr,
        (void (*)(void *)) nullptr,
        (void (*)(void)) nullptr,
        (void (*)(void)) nullptr
};
