#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __XXDF_COMPONENT_FLAG__
#define __XXDF_COMPONENT_FLAG__

#include "common/VSDK.h"

enum XxdfComponentFlag{ DIFFUSE_COMPONENT = 1, GLOSSY_COMPONENT = 2, SPECULAR_COMPONENT = 4
};

class XxdfComponentFlagInfo{ public:
    enum{
        XXDF_COMPONENTS = 3,
        NO_COMPONENTS = 0,
        ALL_COMPONENTS = DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT
    };
};

#endif
