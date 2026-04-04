#ifndef __OPTION_CORE_PARSER__
#define __OPTION_CORE_PARSER__

#include "common/commandLineOptions/TypedOption.h"

template<typename TOptionBase>
class OptionParser {
  public:
    static bool parse(int *argc, char **argv, TOptionBase *registry, int registryCount);
};

#include "common/commandLineOptions/OptionParser.txx"

#endif
