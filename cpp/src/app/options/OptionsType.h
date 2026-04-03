/**
Command line options type registry
*/

#ifndef __OPTIONS_TYPE__
#define __OPTIONS_TYPE__

#include "app/options/CommandLineOptions.h"
#include "app/options/EnumDesc.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"

class OptionsType final {
  public:
    OptionsType();

    CommandLineOptions intType;
    CommandLineOptions boolType;
    CommandLineOptions setTrueType;
    CommandLineOptions setFalseType;
    CommandLineOptions stringType;
    CommandLineOptions floatType;
    CommandLineOptions vectorType;
    CommandLineOptions rgbType;
    CommandLineOptions xyType;

  private:
    static EnumDesc boolTable[5];

    int dummyInt;
    char *dummyString;
    int dummyTrue;
    int dummyFalse;
    float dummyFloat;
    Vector3D dummyVector3D;
    ColorRgb dummyRgb;
    float dummyCieXy[2];
};

#endif
