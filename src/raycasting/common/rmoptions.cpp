/* rmoptions.C: Options defaults and parsing for raymatting */

#include "shared/options.h"
#include "raycasting/common/rmoptions.h"

void RM_Defaults() {
    rms.filter = RM_TENT_FILTER;
    rms.samplesPerPixel = 8;
}


/*** Enum Option types ***/

static ENUMDESC rmPixelFilters[] = {
        {RM_BOX_FILTER,    "box",              2},
        {RM_TENT_FILTER,   "tent",             2},
        {RM_GAUSS_FILTER,  "gaussian 1/sqrt2", 2},
        {RM_GAUSS2_FILTER, "gaussian 1/2",     2},
        {0, nullptr,                              0}
};
MakeEnumOptTypeStruct(rmPixelFilterTypeStruct, rmPixelFilters);
#define TrmPixelFilter (&rmPixelFilterTypeStruct)

static CMDLINEOPTDESC rmOptions[] =
        {
                {"-rm-samples-per-pixel", 6, Tint,           &rms.samplesPerPixel, DEFAULT_ACTION,
                        "-rm-samples-per-pixel <number>\t: eye-rays per pixel"},
                {"-rm-pixel-filter",      7, TrmPixelFilter, &rms.filter,          DEFAULT_ACTION,
                        "-rm-pixel-filter <type>\t: Select filter - \"box\", \"tent\", \"gaussian 1/sqrt2\", \"gaussian 1/2\""},
                {nullptr,                    0, TYPELESS, nullptr,                       DEFAULT_ACTION,
                        nullptr}
        };

void RM_ParseOptions(int *argc, char **argv) {
    ParseOptions(rmOptions, argc, argv);
}
