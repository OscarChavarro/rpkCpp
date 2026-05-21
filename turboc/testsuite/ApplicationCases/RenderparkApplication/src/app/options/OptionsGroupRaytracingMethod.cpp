#include <string.h>

#include "vsdk/common/commandLineOptions/OptionParser.h"
#include "vsdk/common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRaytracingMethod.h"

char *OptionsGroupRaytracingMethod::raytracingMethodsString = NULL;
char *OptionsGroupRaytracingMethod::rayTracerName = NULL;

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
    char *raytracingMethodName = NULL;
    TypedOption<char *> raytracingMethodOpt("-raytracing-method", &raytracingMethodName, 1, OptionsGroupRaytracingMethod::mainRayTracingOption, NULL);
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
