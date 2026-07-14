#ifndef OPTION_CORE_PARSER__
#define OPTION_CORE_PARSER__

#include "vsdk/toolkit/common/commandLineOptions/OptionGroup.h"

template<typename TOptionBase>
class OptionParser {
  public:
    static bool parse(int *argc, char **argv, TOptionBase *registry, int registryCount, void *context = nullptr);
    static bool parse(int *argc, char **argv, OptionGroupT<TOptionBase> *groups, int groupCount, void *context = nullptr);
};

#include "vsdk/toolkit/common/commandLineOptions/OptionParser.txx"

#endif
