#ifndef __IDENTITY_TONE_MAP__
#define __IDENTITY_TONE_MAP__

#include "tonemap/ToneMap.h"

class IdentityToneMap: public ToneMap{ public:
    IdentityToneMap();
    ~IdentityToneMap();

    void init(const ToneMappingContext &toneMapOptions);
    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
