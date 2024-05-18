#include "app/BatchOptions.h"

BatchOptions::BatchOptions() {
    iterations = 1;
    radianceImageFileNameFormat = "";
    radianceModelFileNameFormat = "";
    saveModulo = 10;
    raytracingImageFileName = "";
    timings = false;
}

BatchOptions::~BatchOptions() {
}
