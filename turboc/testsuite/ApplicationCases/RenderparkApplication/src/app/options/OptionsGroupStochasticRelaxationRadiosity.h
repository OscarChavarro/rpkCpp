#ifndef OPTNS_GRP_STCHS_RLXTN_RDSTY
#define OPTNS_GRP_STCHS_RLXTN_RDSTY

#include "app/options/EnumDesc.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"

class OptsGrpStochRelaxRad{ public:
    static void parse( int *argc, char **argv, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState);

  private:
    template<typename T>
    class EnumBinding{ public:
        T *target;
        const EnumDesc *values;
    };

    static EnumDesc approximateValues[];
    static EnumDesc clusteringValues[];
    static EnumDesc sequenceValues[];
    static EnumDesc showWhatValues[];

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static bool parseSequenceBinding(int argc, char **argv, EnumBinding<Sampler4DSequence> &binding);
    static bool parseApproximationBinding(int argc, char **argv, EnumBinding<StochRaytrApprx> &binding);
    static bool parseClusteringBinding(int argc, char **argv, EnumBinding<HierarchyClusteringMode> &binding);
    static bool parseShowBinding(int argc, char **argv, EnumBinding<WhatToShow> &binding);
    static bool parseBoolInt(int argc, char **argv, int &value);
};

#endif
