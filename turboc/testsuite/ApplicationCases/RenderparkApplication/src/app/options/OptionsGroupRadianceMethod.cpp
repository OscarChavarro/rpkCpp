#include <string.h>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRadianceMethod.h"

void
OptionsGroupRadianceMethod::radianceMethodParseOptions(
    int *argc,
    char **argv,
    char *radianceMethodsStringOut)
{
    char *radianceMethodName = NULL;
    TypedOption<char *> radianceMethodOpt("-radiance-method", &radianceMethodName, 1, NULL, NULL);
    OptionBase radianceOptions[] = {
        REGISTER_OPTION(char *, radianceMethodOpt, 4)
    };
    OptionGroup radianceGroups[] = {
        OptionGroup("radiance", radianceOptions, 1)
    };
    OptionParser<OptionBase>::parse(argc, argv, radianceGroups, 1);

    if ( radianceMethodsStringOut != NULL ) {
        if ( radianceMethodName != NULL ) {
            strcpy(radianceMethodsStringOut, radianceMethodName);
        } else {
            radianceMethodsStringOut[0] = '\0';
        }
    }
}
