/**
Implements option handling for bidirectional path tracing
*/

#include "common/options.h"
#include "render/bidiroptions.h"

/**
User interface links
*/
void
biDirectionalPathDefaults() {
    GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel = 1;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth = 2;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumPathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth = 7;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLe = true;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLD = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doLI = false;

    // Weighted not in UI
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doWeighted = false;

    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.leRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.ldRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.liRegExp, MAX_REGEXP_SIZE, "(LX)(G|S)(X)*(EX),(LX)(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.wleRegExp, MAX_REGEXP_SIZE, "(LX)(DR)(X)*(EX)");
    snprintf(GLOBAL_rayTracing_biDirectionalPath.basecfg.wldRegExp, MAX_REGEXP_SIZE, "(LX)(X)*(EX)");

    // Not in UI yet
    GLOBAL_rayTracing_biDirectionalPath.saveSubsequentImages = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.eliminateSpikes = false;
    GLOBAL_rayTracing_biDirectionalPath.basecfg.doDensityEstimation = false;
    GLOBAL_rayTracing_biDirectionalPath.baseFilename[0] = '\0';
}

MakeNStringTypeStruct(RegExpStringType, MAX_REGEXP_SIZE);
#define TregexpString (&RegExpStringType)

static CommandLineOptionDescription globalBiDirectionalOptions[] = {
    {"-bidir-samples-per-pixel", 8, Tint,         &GLOBAL_rayTracing_biDirectionalPath.basecfg.samplesPerPixel,       DEFAULT_ACTION,
    "-bidir-samples-per-pixel <number> : eye-rays per pixel"},
    {"-bidir-no-progressive", 11, Tsetfalse,      &GLOBAL_rayTracing_biDirectionalPath.basecfg.progressiveTracing,    DEFAULT_ACTION,
    "-bidir-no-progressive          \t: don't do progressive image refinement"},
    {"-bidir-max-eye-path-length", 12, Tint,      &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumEyePathDepth,   DEFAULT_ACTION,
    "-bidir-max-eye-path-length <number>: maximum eye path length"},
    {"-bidir-max-light-path-length", 12, Tint,    &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumLightPathDepth, DEFAULT_ACTION,
    "-bidir-max-light-path-length <number>: maximum light path length"},
    {"-bidir-max-path-length", 12, Tint,          &GLOBAL_rayTracing_biDirectionalPath.basecfg.maximumPathDepth,      DEFAULT_ACTION,
    "-bidir-max-path-length <number>\t: maximum combined path length"},
    {"-bidir-min-path-length", 12, Tint,          &GLOBAL_rayTracing_biDirectionalPath.basecfg.minimumPathDepth,      DEFAULT_ACTION,
    "-bidir-min-path-length <number>\t: minimum path length before russian roulette"},
    {"-bidir-no-light-importance", 11, Tsetfalse, &GLOBAL_rayTracing_biDirectionalPath.basecfg.sampleImportantLights, DEFAULT_ACTION,
    "-bidir-no-light-importance     \t: sample lights based on power, ignoring their importance"},
    {"-bidir-use-regexp", 12, Tsettrue,           &GLOBAL_rayTracing_biDirectionalPath.basecfg.useSpars,              DEFAULT_ACTION,
    "-bidir-use-regexp\t: use regular expressions for path evaluation"},
    {"-bidir-use-emitted", 12, Tbool,             &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLe,                  DEFAULT_ACTION,
    "-bidir-use-emitted <yes|no>\t: use reg exp for emitted radiance"},
    {"-bidir-rexp-emitted", 13, TregexpString,    GLOBAL_rayTracing_biDirectionalPath.basecfg.leRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-emitted <string>\t: reg exp for emitted radiance"},
    {"-bidir-reg-direct", 12, Tbool,              &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLD,                  DEFAULT_ACTION,
    "-bidir-reg-direct <yes|no>\t: use reg exp for stored direct illumination (galerkin!)"},
    {"-bidir-rexp-direct", 13, TregexpString,     GLOBAL_rayTracing_biDirectionalPath.basecfg.ldRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-direct <string>\t: reg exp for stored direct illumination"},
    {"-bidir-reg-indirect", 12, Tbool,            &GLOBAL_rayTracing_biDirectionalPath.basecfg.doLI,                  DEFAULT_ACTION,
    "-bidir-reg-indirect <yes|no>\t: use reg exp for stored indirect illumination (galerkin!)"},
    {"-bidir-rexp-indirect", 13, TregexpString,   GLOBAL_rayTracing_biDirectionalPath.basecfg.liRegExp,               DEFAULT_ACTION,
    "-bidir-rexp-indirect <string>\t: reg exp for stored indirect illumination"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION,nullptr}
};

void
biDirectionalPathParseOptions(int *argc, char **argv) {
    parseOptions(globalBiDirectionalOptions, argc, argv);
}
