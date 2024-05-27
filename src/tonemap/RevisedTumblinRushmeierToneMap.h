#ifndef __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__
#define __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__

#include "tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap final : public ToneMap {
  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap() final;
    void defaults() final;
};

extern OldToneMap GLOBAL_toneMap_revisedTumblinRushmeier;

#endif
