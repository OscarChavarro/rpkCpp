#include <cstring>

#include "java/util/Formatter.h"

#include "options/OptionsGroupRaytracingMethod.h"
#include "options/OptionsGroupRaytracing.h"

void
OptionsGroupRaytracing::makeMethodsHelpMessage(char *buffer) {
    java::Formatter::format(buffer, 1000,
         "-raytracing-method <method>: set pixel-based radiance computation method\n"
         "\tmethods: none                 no pixel-based radiance computation\n"
         "\t         StochasticRaytracing Stochastic Raytracing & Final Gathers (default)\n"
         "\t         BidirectionalPathTra Bidirectional Path Tracing\n"
         "\t         RayCasting           Ray Casting\n"
         "\t         RayMatting           Ray Matting");
}

void
OptionsGroupRaytracing::parse(
    int *argc,
    char **argv,
    char *rayTracerName)
{
    char helpMessage[1000];

    OptionsGroupRaytracing::makeMethodsHelpMessage(helpMessage);
    strcpy(rayTracerName, "none");
    OptionsGroupRaytracingMethod::rayTracingParseOptions(argc, argv, helpMessage, rayTracerName);
}
