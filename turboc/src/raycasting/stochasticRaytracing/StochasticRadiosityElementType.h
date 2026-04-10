
#include "common/VSDK.h"
#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef STCHS_RDSTY_ELMNT_TYPE
#define STCHS_RDSTY_ELMNT_TYPE

enum StochasticRadiosityElementType{ ET_TRIANGLE = 0, ET_QUAD = 1
};

class StochRadElemTypeInfo{ public:
    static const int NUMBER_OF_ELEMENT_TYPES = 2;
};

#endif
