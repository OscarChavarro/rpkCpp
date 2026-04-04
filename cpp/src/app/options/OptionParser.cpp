#include "app/options/OptionParser.h"

bool
OptionParser::parse(int *argc, char **argv, OptionBase *registry, int registryCount) {
    return parseOptionBaseRegistryInPlace(registry, registryCount, argc, argv);
}
