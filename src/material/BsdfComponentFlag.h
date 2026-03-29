#ifndef __BSDF_COMPONENT_FLAG__
#define __BSDF_COMPONENT_FLAG__

#include "material/XxdfComponentFlag.h"

class BsdfComponentFlag {
  public:
    static constexpr int
    bsdfIndexToComp(const int index) {
        return 1 << index;
    }

    static constexpr int
    getBrdfFlags(const int bsflags) {
        return bsflags & ALL_COMPONENTS;
    }

    static constexpr int
    getBtdfFlags(const int bsflags) {
        return (bsflags >> XXDF_COMPONENTS) & ALL_COMPONENTS;
    }
};

#endif
