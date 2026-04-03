#ifndef __PARSER_CONFIG__
#define __PARSER_CONFIG__

#include "scene/RadianceMethod.h"

class ParserConfig {
  public:
    RadianceMethod *radianceMethod;
    bool singleSided;
    int numberOfQuarterCircleDivisions;
    bool monochrome;

    ParserConfig();
};

#endif
