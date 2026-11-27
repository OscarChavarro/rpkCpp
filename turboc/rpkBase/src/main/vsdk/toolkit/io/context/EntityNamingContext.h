#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __ENTITY_NAMING_CONTEXT__
#define __ENTITY_NAMING_CONTEXT__

#include "vsdk/toolkit/common/VSDK.h"

class EntityNamingContext{ public:
    enum{
        MGF_MAXIMUM_ENTITY_NAME_LENGTH = 6
    };
};

#endif
