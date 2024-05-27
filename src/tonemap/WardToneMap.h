#ifndef __WARD_TONE_MAP__
#define __WARD_TONE_MAP__

#include "tonemap/ToneMappingContext.h"

class WardToneMap final : public ToneMap {
  public:
    WardToneMap();
    ~WardToneMap() final;
    void defaults() final;
};

extern OldToneMap GLOBAL_toneMap_ward;

#endif
