#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __POLYGON_CLIP_RESULT__
#define __POLYGON_CLIP_RESULT__

#include "vsdk/toolkit/common/VSDK.h"

class PolygonClipResultInfo{ public:
    enum{
        MAXIMUM_SIDES_PER_POLYGON = 10
    };
};

enum PolygonClipResult{ POLY_CLIP_OUT = 0, POLY_CLIP_PARTIAL = 1, POLY_CLIP_IN = 2
};

#endif
