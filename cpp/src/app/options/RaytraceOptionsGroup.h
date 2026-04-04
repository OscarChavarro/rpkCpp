#ifndef __RAYTRACE_OPTIONS_GROUP__
#define __RAYTRACE_OPTIONS_GROUP__


class RaytraceOptionsGroup final {
  public:
    static void parse(
        int *argc,
        char **argv,
        char *rayTracerName);

  private:
    static void makeMethodsHelpMessage(char *buffer);
};

#endif
