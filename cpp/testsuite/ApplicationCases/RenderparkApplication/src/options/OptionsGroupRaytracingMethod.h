#ifndef __OPTIONS_GROUP_RAYTRACING_METHOD__
#define __OPTIONS_GROUP_RAYTRACING_METHOD__

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
