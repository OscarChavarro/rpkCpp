/**
Implements option handling for bidirectional path tracing
*/

#include "shared/options.h"
#include "raycasting/common/bidiroptions.h"

/**
User interface links
*/
void
biDirectionalPathDefaults() {
    bidir.basecfg.samplesPerPixel = 1;
    bidir.basecfg.progressiveTracing = true;
    bidir.basecfg.minimumPathDepth = 2;
    bidir.basecfg.maximumPathDepth = 7;
    bidir.basecfg.maximumEyePathDepth = 7;
    bidir.basecfg.maximumLightPathDepth = 7;
    bidir.basecfg.sampleImportantLights = true;
    bidir.basecfg.useSpars = false;
    bidir.basecfg.doLe = true;
    bidir.basecfg.doLD = false;
    bidir.basecfg.doLI = false;

    // Weighted not in UI
    bidir.basecfg.doWeighted = false;

    snprintf(bidir.basecfg.leRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");
    snprintf(bidir.basecfg.ldRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(bidir.basecfg.liRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(bidir.basecfg.wleRegExp, MAX_REGEXP_SIZE, "(LX)(DR)(X)*(EX)");
    snprintf(bidir.basecfg.wldRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");

    // Not in UI yet
    bidir.saveSubsequentImages = false;
    bidir.basecfg.eliminateSpikes = false;
    bidir.basecfg.doDensityEstimation = false;
    bidir.baseFilename[0] = '\0';
}

MakeNStringTypeStruct(RegExpStringType, MAX_REGEXP_SIZE);
#define TregexpString (&RegExpStringType)

static CMDLINEOPTDESC globalBiDirectionalOptions[] = {
    {"-bidir-samples-per-pixel", 8, Tint, &bidir.basecfg.samplesPerPixel,       DEFAULT_ACTION,
    "-bidir-samples-per-pixel <number> : eye-rays per pixel"},
    {"-bidir-no-progressive", 11, Tsetfalse, &bidir.basecfg.progressiveTracing,    DEFAULT_ACTION,
    "-bidir-no-progressive          \t: don't do progressive image refinement"},
    {"-bidir-max-eye-path-length", 12, Tint, &bidir.basecfg.maximumEyePathDepth,   DEFAULT_ACTION,
    "-bidir-max-eye-path-length <number>: maximum eye path length"},
    {"-bidir-max-light-path-length", 12, Tint, &bidir.basecfg.maximumLightPathDepth, DEFAULT_ACTION,
    "-bidir-max-light-path-length <number>: maximum light path length"},
    {"-bidir-max-path-length", 12, Tint, &bidir.basecfg.maximumPathDepth,      DEFAULT_ACTION,
    "-bidir-max-path-length <number>\t: maximum combined path length"},
    {"-bidir-min-path-length", 12, Tint, &bidir.basecfg.minimumPathDepth,      DEFAULT_ACTION,
    "-bidir-min-path-length <number>\t: minimum path length before russian roulette"},
    {"-bidir-no-light-importance", 11, Tsetfalse, &bidir.basecfg.sampleImportantLights, DEFAULT_ACTION,
    "-bidir-no-light-importance     \t: sample lights based on power, ignoring their importance"},
    {"-bidir-use-regexp", 12, Tsettrue, &bidir.basecfg.useSpars,              DEFAULT_ACTION,
    "-bidir-use-regexp\t: use regular expressions for path evaluation"},
    {"-bidir-use-emitted", 12, Tbool, &bidir.basecfg.doLe,                  DEFAULT_ACTION,
    "-bidir-use-emitted <yes|no>\t: use reg exp for emitted radiance"},
    {"-bidir-rexp-emitted", 13, TregexpString, bidir.basecfg.leRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-emitted <string>\t: reg exp for emitted radiance"},
    {"-bidir-reg-direct", 12, Tbool, &bidir.basecfg.doLD,                  DEFAULT_ACTION,
    "-bidir-reg-direct <yes|no>\t: use reg exp for stored direct illumination (galerkin!)"},
    {"-bidir-rexp-direct", 13, TregexpString, bidir.basecfg.ldRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-direct <string>\t: reg exp for stored direct illumination"},
    {"-bidir-reg-indirect", 12, Tbool, &bidir.basecfg.doLI,                  DEFAULT_ACTION,
    "-bidir-reg-indirect <yes|no>\t: use reg exp for stored indirect illumination (galerkin!)"},
    {"-bidir-rexp-indirect", 13, TregexpString, bidir.basecfg.liRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-indirect <string>\t: reg exp for stored indirect illumination"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION,nullptr}
};

void
biDirectionalPathParseOptions(int *argc, char **argv) {
    parseOptions(globalBiDirectionalOptions, argc, argv);
}
