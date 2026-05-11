#ifndef OPTIONS_GROUP_STOCHASTIC_RELAXATION_RADIOSITY__
#define OPTIONS_GROUP_STOCHASTIC_RELAXATION_RADIOSITY__

#include "options/EnumDesc.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"

class OptionsGroupStochasticRelaxationRadiosity final {
  public:
    static void parse(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState);

  private:
    template<typename T>
    class EnumBinding {
      public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc approximateValues[];
    static EnumDesc clusteringValues[];
    static EnumDesc sequenceValues[];
    static EnumDesc showWhatValues[];

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static bool parseBoolInt(int argc, char **argv, int &value);
};

#endif
