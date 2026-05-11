#include <cstring>

#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "options/OptionsGroupRadianceMethod.h"

void
OptionsGroupRadianceMethod::radianceMethodParseOptions(
    int *argc,
    char **argv,
    char *radianceMethodsStringOut)
{
    char *radianceMethodName = nullptr;
    TypedOption<char *> radianceMethodOpt = {"-radiance-method", &radianceMethodName, 1, nullptr, nullptr};
    OptionBase radianceOptions[] = {
        REGISTER_OPTION(char *, radianceMethodOpt, 4)
    };
    OptionGroup radianceGroups[] = {
        OptionGroup("radiance", radianceOptions, 1)
    };
    OptionParser<OptionBase>::parse(argc, argv, radianceGroups, 1);

    if ( radianceMethodsStringOut != nullptr ) {
        if ( radianceMethodName != nullptr ) {
            strcpy(radianceMethodsStringOut, radianceMethodName);
        } else {
            radianceMethodsStringOut[0] = '\0';
        }
    }
}
