#ifndef __SPAR_LIST__
#define __SPAR_LIST__

#include "common/dataStructures/CircularList.h"

class BiPath;
class ColorRgb;
class Spar;
class SparConfig;

class SparList final : public CircularList<Spar *> {
  public:
    void
    handlePath(
        SparConfig *config,
        BiPath *path,
        ColorRgb *fRad,
        ColorRgb *fBpt);
    ~SparList() final;
};

typedef CircularListIterator<Spar *> CSparListIter;

#endif
