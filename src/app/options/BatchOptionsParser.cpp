#include "app/options/BatchOptionsParser.h"
#include "app/options/CommandLine.h"

void
BatchOptionsParser::parse(
    int *argc,
    char **argv,
    BatchOptions &batchOptions,
    OptionsType &optionTypes)
{
    CommandLine::batchParseOptions(argc, argv, &batchOptions, optionTypes);
}
