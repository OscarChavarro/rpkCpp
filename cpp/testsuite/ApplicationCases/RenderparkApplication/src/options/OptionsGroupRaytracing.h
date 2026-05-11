#ifndef RAYTRACING_OPTIONS_GROUP__
#define RAYTRACING_OPTIONS_GROUP__


class OptionsGroupRaytracing final {
  public:
    static void parse(
        int *argc,
        char **argv,
        char *rayTracerName);

  private:
    static void makeMethodsHelpMessage(char *buffer);
};

#endif
