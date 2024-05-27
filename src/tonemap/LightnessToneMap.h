#ifndef __TONE_MAP_LIGHTNESS__
#define __TONE_MAP_LIGHTNESS__

#include "tonemap/ToneMap.h"

class LightnessToneMap final : public ToneMap {
  public:
    LightnessToneMap();
    ~LightnessToneMap() final;
    void init() final;
};

extern OldToneMap GLOBAL_toneMap_lightness;

#endif
