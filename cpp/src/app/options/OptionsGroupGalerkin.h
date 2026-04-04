#ifndef __COMMAND_LINE_GALERKIN_OPTIONS_GROUP__
#define __COMMAND_LINE_GALERKIN_OPTIONS_GROUP__

class OptionsGroupGalerkin final {
  public:
    static void galerkinParseOptions(int *argc, char **argv);

  private:
    static int trueValue;
    static int falseValue;

    static void iterationMethodOption(char *&value);
    static void hierarchicalOption(int &value);
    static void lazyOption(int &value);
    static void clusteringOption(int &value);
    static void importanceOption(int &value);
    static void ambientOption(int &value);
};

#endif
