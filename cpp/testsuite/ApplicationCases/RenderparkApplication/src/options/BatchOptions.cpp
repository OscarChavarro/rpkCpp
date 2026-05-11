#include "options/BatchOptions.h"

BatchOptions::BatchOptions() {
    exportBinary = false;
    binaryOutputFilename = "";
    importBinary = false;
    binaryInputFilename = "";
    iterations = 1;
    radianceImageFileNameFormat = "";
    radianceModelFileNameFormat = "";
    saveModulo = 10;
    raytracingImageFileName = "";
    timings = false;
}

BatchOptions::~BatchOptions() {
}
