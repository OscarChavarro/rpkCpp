#ifndef __OPTION_CORE_PARSER__
#define __OPTION_CORE_PARSER__

#include "common/commandLineOptions/TypedOption.h"

template<typename TOptionBase>
class OptionGroupT {
  public:
    OptionGroupT():
        name(nullptr),
        options(nullptr),
        count(0) {
    }

    OptionGroupT(const char *groupName, TOptionBase *groupOptions, int groupCount):
        name(groupName),
        options(groupOptions),
        count(groupCount) {
    }

    const char *name;
    TOptionBase *options;
    int count;
};

typedef OptionGroupT<OptionBase> OptionGroup;

template<typename TOptionBase>
class OptionParser {
  public:
    static bool parse(int *argc, char **argv, TOptionBase *registry, int registryCount, void *context = nullptr);
    static bool parse(int *argc, char **argv, OptionGroupT<TOptionBase> *groups, int groupCount, void *context = nullptr);
};

#include "common/commandLineOptions/OptionParser.txx"

#endif
