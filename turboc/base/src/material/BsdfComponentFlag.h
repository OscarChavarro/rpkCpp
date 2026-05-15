#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __BSDF_COMPONENT_FLAG__
#define __BSDF_COMPONENT_FLAG__

#include "material/XxdfComponentFlag.h"

class BsdfComponentFlag {
  public:
    static int
    bsdfIndexToComp(const int index) {
        return 1 << index;
    }

    static int
    getBrdfFlags(const int bsflags) {
        return bsflags & XxdfComponentFlagInfo::ALL_COMPONENTS;
    }

    static int
    getBtdfFlags(const int bsflags) {
        return (bsflags >> XxdfComponentFlagInfo::XXDF_COMPONENTS) & XxdfComponentFlagInfo::ALL_COMPONENTS;
    }
};

#endif
