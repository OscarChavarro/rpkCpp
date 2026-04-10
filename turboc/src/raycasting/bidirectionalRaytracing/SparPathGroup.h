#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __SPAR_PATH_GROUP__
#define __SPAR_PATH_GROUP__

#include "common/VSDK.h"

#define SPAR_MAX_PATH_GROUPS 2

enum SparPathGroup{ DISJOINT_GROUP = 0, LD_GROUP = 1
};

#endif
