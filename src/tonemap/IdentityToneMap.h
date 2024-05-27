#ifndef __IDENTITY_TONE_MAP__
#define __IDENTITY_TONE_MAP__

#include "tonemap/ToneMap.h"

class IdentityToneMap final : public ToneMap {
  public:
    IdentityToneMap();
    ~IdentityToneMap() final;
    void defaults() final;
};

extern OldToneMap GLOBAL_toneMap_dummy;

#endif
