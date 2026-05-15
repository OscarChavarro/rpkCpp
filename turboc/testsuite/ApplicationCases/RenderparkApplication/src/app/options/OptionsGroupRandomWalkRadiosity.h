#ifndef OPTNS_GRP_RNDM_WALK_RDSTY
#define OPTNS_GRP_RNDM_WALK_RDSTY

#include "app/options/EnumDesc.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

class OptionsGroupRandomWalkRadiosity{ public:
    static void parse( int *argc, char **argv, StochasticRelaxation &stochasticRelaxationState);

  private:
    template<typename T>
    class EnumBinding{ public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc approximateValues[];
    static EnumDesc sequenceValues[];
    static EnumDesc estimatorTypeValues[];
    static EnumDesc estimatorKindValues[];

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static bool parseBoolInt(int argc, char **argv, int &value);
};

#endif
