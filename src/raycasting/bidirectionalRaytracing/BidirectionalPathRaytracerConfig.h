/**
Encapsulates option handling for bidirectional path tracing
*/

#ifndef __BI_DIR_OPTIONS__
#define __BI_DIR_OPTIONS__

#include "render/ScreenBuffer.h"

#define MAX_REGEXP_SIZE 100

/**
Contains basic config options used throughout the complete BPT code. typically two
instances are used. One persistent, containing current GUI options and one non
persistent copy when rendering an image
*/
class BidirectionalPathRaytracerConfig {
  public:
    // Path depth config
    int maximumEyePathDepth;
    int maximumLightPathDepth;
    int maximumPathDepth;
    int minimumPathDepth;

    // Sampling details
    int samplesPerPixel;
    long totalSamples;
    int sampleImportantLights;
    int progressiveTracing;
    int eliminateSpikes;

    // SPaR details (usage of reg exps)
    int useSpars;
    int doLe;
    int doLD;
    int doLI;

    char leRegExp[MAX_REGEXP_SIZE];
    char ldRegExp[MAX_REGEXP_SIZE];
    char liRegExp[MAX_REGEXP_SIZE];

    int doWeighted; // Use weighted multi-pass method for a part of le and ld transport
    char wleRegExp[MAX_REGEXP_SIZE];
    char wldRegExp[MAX_REGEXP_SIZE];

    // These values are not yet in the presets or UI!
    int doDensityEstimation;
};

#endif
