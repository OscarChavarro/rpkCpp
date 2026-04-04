#include <cstring>

#include "java/util/Formatter.h"

#include "app/options/CommandLine.h"
#include "app/options/RaytraceOptionsGroup.h"

void
RaytraceOptionsGroup::makeMethodsHelpMessage(char *buffer) {
    java::Formatter::format(buffer, 1000,
         "-raytracing-method <method>: set pixel-based radiance computation method\n"
         "\tmethods: none                 no pixel-based radiance computation\n"
         "\t         StochasticRaytracing Stochastic Raytracing & Final Gathers (default)\n"
         "\t         BidirectionalPathTra Bidirectional Path Tracing\n"
         "\t         RayCasting           Ray Casting\n"
         "\t         RayMatting           Ray Matting");
}

void
RaytraceOptionsGroup::parse(
    int *argc,
    char **argv,
    char *rayTracerName)
{
    char helpMessage[1000];

    RaytraceOptionsGroup::makeMethodsHelpMessage(helpMessage);
    strcpy(rayTracerName, "none");
    CommandLine::rayTracingParseOptions(argc, argv, helpMessage, rayTracerName);
}
