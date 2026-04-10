#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __ENTITY_CONTEXT_INFO__
#define __ENTITY_CONTEXT_INFO__

#include "common/VSDK.h"

class EntityNamingContext{ public:
    enum{
        MGF_MAXIMUM_ENTITY_NAME_LENGTH = 6
    };
};

#endif
