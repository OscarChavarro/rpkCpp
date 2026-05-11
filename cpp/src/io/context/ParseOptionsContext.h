#ifndef PARSER_CONFIG__
#define PARSER_CONFIG__

#include "scene/RadianceMethod.h"

class ParseOptionsContext {
  public:
    RadianceMethod *radianceMethod;
    bool singleSided;
    int numberOfQuarterCircleDivisions;
    bool monochrome;

    ParseOptionsContext();
};

#endif
