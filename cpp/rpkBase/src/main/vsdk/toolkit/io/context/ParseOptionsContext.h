#ifndef PARSER_CONFIG__
#define PARSER_CONFIG__

#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class ParseOptionsContext {
  public:
    RadianceMethod *radianceMethod;
    bool singleSided;
    int numberOfQuarterCircleDivisions;
    bool monochrome;

    ParseOptionsContext();

    // RGB color space MGF colors (CIE xy) are converted into
    const Cie &getColorSpace() const { return colorSpace; }
    void setColorSpace(const Cie &inColorSpace) { colorSpace = inColorSpace; }

  private:
    Cie colorSpace;
};

#endif
