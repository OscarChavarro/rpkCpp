#include <cstring>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRaytracingMethod.h"

char *OptionsGroupRaytracingMethod::raytracingMethodsString = nullptr;
char *OptionsGroupRaytracingMethod::rayTracerName = nullptr;

void
OptionsGroupRaytracingMethod::mainRayTracingOption(char *&name) {
    strcpy(rayTracerName, name);
}

void
OptionsGroupRaytracingMethod::rayTracingParseOptions(
        int *argc,
        char **argv,
        char raytracingMethodsStringOut[],
        char *rayTracerNameOut)
{
    char *raytracingMethodName = nullptr;
    TypedOption<char *> raytracingMethodOpt = {"-raytracing-method", &raytracingMethodName, 1, OptionsGroupRaytracingMethod::mainRayTracingOption, nullptr};
    OptionBase raytracingOptions[] = {
        REGISTER_OPTION(char *, raytracingMethodOpt, 4)
    };

    OptionsGroupRaytracingMethod::rayTracerName = rayTracerNameOut;
    OptionsGroupRaytracingMethod::raytracingMethodsString = raytracingMethodsStringOut;
    OptionGroup raytracingGroups[] = {
        OptionGroup("raytracing", raytracingOptions, 1)
    };
    OptionParser<OptionBase>::parse(argc, argv, raytracingGroups, 1);
}
