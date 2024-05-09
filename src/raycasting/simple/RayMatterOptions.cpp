#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

/**
Options defaults and parsing for ray matting
*/

#include "common/options.h"
#include "raycasting/simple/RayMatterOptions.h"

void
rayMattingDefaults() {
    GLOBAL_rayCasting_rayMatterState.filter = RM_TENT_FILTER;
    GLOBAL_rayCasting_rayMatterState.samplesPerPixel = 8;
}

static ENUMDESC globalRayMatterPixelFilters[] = {
    {RM_BOX_FILTER, "box", 2},
    {RM_TENT_FILTER, "tent", 2},
    {RM_GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RM_GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};
MakeEnumOptTypeStruct(rmPixelFilterTypeStruct, globalRayMatterPixelFilters);
#define TrmPixelFilter (&rmPixelFilterTypeStruct)

static CommandLineOptionDescription globalRayMatterOptions[] =
{
    {"-rm-samples-per-pixel", 6, &GLOBAL_options_intType, &GLOBAL_rayCasting_rayMatterState.samplesPerPixel, DEFAULT_ACTION,
     "-rm-samples-per-pixel <number>\t: eye-rays per pixel"},
    {"-rm-pixel-filter", 7, TrmPixelFilter, &GLOBAL_rayCasting_rayMatterState.filter, DEFAULT_ACTION,
     "-rm-pixel-filter <type>\t: Select filter - \"box\", \"tent\", \"gaussian 1/sqrt2\", \"gaussian 1/2\""},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
rayMattingParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalRayMatterOptions, argc, argv);
}

#endif
