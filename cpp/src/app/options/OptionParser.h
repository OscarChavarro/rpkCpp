#ifndef __OPTION_PARSER__
#define __OPTION_PARSER__

#include "app/options/TypedOption.h"

struct OptionParser {
    static bool parse(int *argc, char **argv, OptionBase *registry, int registryCount);
};

#endif
