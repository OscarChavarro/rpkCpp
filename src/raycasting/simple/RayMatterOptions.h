/**
Options and global state vars for ray matting
*/

#ifndef __RAY_MATTER_OPTIONS__
#define __RAY_MATTER_OPTIONS__

#include "render/ScreenBuffer.h"

enum RM_FILTER_OPTION {
    RM_BOX_FILTER,
    RM_TENT_FILTER,
    RM_GAUSS_FILTER,
    RM_GAUSS2_FILTER
};

class RayMattingState {
public:
    int samplesPerPixel; // Pixel sampling
    RM_FILTER_OPTION filter; // Pixel filter
};

extern RayMattingState GLOBAL_rayCasting_rayMatterState;

void rayMattingDefaults();
void rayMattingParseOptions(int *argc, char **argv);

#endif
