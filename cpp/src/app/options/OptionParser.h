#ifndef __OPTION_PARSER__
#define __OPTION_PARSER__

#include "app/options/OptionRegistry.h"

struct OptionParser {
    static void configureLegacy(LegacyOptionRegistry registry, int *argc);
    static bool parse(int argc, char **argv);

  private:
    static LegacyOptionRegistry legacyRegistry;
    static int *argumentCount;

    static OptionValueWrapper valueOrDummy(CommandLineOptionDescription *opt);
    static bool typeConsumesArgument(const CommandLineOptions *type);
    static bool processOne();
    static void compactArguments(char **argv, int originalArgc);
};

#endif
