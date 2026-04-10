#ifndef __SPAR_LIST__
#define __SPAR_LIST__

#include "common/ColorRgb.h"
#include "common/dataStructures/CircularList.h"
#include "raycasting/bidirectionalRaytracing/BiPath.h"
#include "raycasting/bidirectionalRaytracing/SparConfig.h"

class Spar;

class SparList: public CircularList<Spar *>{ public:
    void
    handlePath( SparConfig *config, BiPath *path, ColorRgb *fRad, ColorRgb *fBpt);
    ~SparList();
};

typedef CircularListIterator<Spar *> CSparListIter;

#endif
