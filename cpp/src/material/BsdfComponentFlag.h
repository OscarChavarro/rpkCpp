#ifndef BSDF_COMPONENT_FLAG__
#define BSDF_COMPONENT_FLAG__

#include "material/XxdfComponentFlag.h"

class BsdfComponentFlag {
  public:
    static constexpr int
    bsdfIndexToComp(const int index) {
        return 1 << index;
    }

    static constexpr int
    getBrdfFlags(const int bsflags) {
        return bsflags & XxdfComponentFlagInfo::ALL_COMPONENTS;
    }

    static constexpr int
    getBtdfFlags(const int bsflags) {
        return (bsflags >> XxdfComponentFlagInfo::XXDF_COMPONENTS) & XxdfComponentFlagInfo::ALL_COMPONENTS;
    }
};

#endif
