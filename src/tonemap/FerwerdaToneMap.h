#ifndef __FERWERDA_TONE_MAP__
#define __FERWERDA_TONE_MAP__

#include "tonemap/ToneMap.h"

class FerwerdaToneMap final : public ToneMap {
  public:
    FerwerdaToneMap();
    ~FerwerdaToneMap() final;
    void defaults() final;
};

extern OldToneMap GLOBAL_toneMap_ferwerda;

#endif
