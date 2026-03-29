#ifndef __PARSER_CONFIG__
#define __PARSER_CONFIG__

class RadianceMethod;

class ParserConfig {
  public:
    RadianceMethod *radianceMethod;
    bool singleSided;
    int numberOfQuarterCircleDivisions;
    bool monochrome;

    ParserConfig();
};

#endif
