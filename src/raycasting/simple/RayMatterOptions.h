/**
Options and global state vars for ray matting
*/

#ifndef __RAY_MATTER_OPTIONS__
#define __RAY_MATTER_OPTIONS__

#include "render/ScreenBuffer.h"

enum RayMatterFilterType {
    BOX_FILTER,
    TENT_FILTER,
    GAUSS_FILTER,
    GAUSS2_FILTER
};

class RayMatterState {
  public:
    int samplesPerPixel; // Pixel sampling
    RayMatterFilterType filter; // Pixel filter
};

extern RayMatterState GLOBAL_rayCasting_rayMatterState;

void rayMattingDefaults();
void rayMattingParseOptions(int *argc, char **argv);

#endif
