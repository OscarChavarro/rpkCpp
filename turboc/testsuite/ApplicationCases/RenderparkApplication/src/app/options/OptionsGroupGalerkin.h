
#include "vsdk/common/VSDK.h"
#ifndef CMMND_LINE_GLRKN_OPTNS_GRP
#define CMMND_LINE_GLRKN_OPTNS_GRP

class OptionsGroupGalerkin{ public:
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
