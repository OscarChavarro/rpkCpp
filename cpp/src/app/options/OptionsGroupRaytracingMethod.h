#ifndef __COMMAND_LINE_RAYTRACING_METHOD_OPTIONS_GROUP__
#define __COMMAND_LINE_RAYTRACING_METHOD_OPTIONS_GROUP__

class OptionsGroupRaytracingMethod final {
  public:
    static void rayTracingParseOptions(
        int *argc,
        char **argv,
        char raytracingMethodsStringOut[],
        char *rayTracerNameOut);

  private:
    static char *raytracingMethodsString;
    static char *rayTracerName;

    static void mainRayTracingOption(char *&value);
};

#endif
