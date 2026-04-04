#include "app/options/BatchOptionsGroup.h"
#include "app/options/CommandLine.h"

void
BatchOptionsGroup::parse(
    int *argc,
    char **argv,
    BatchOptions &batchOptions)
{
    CommandLine::batchParseOptions(argc, argv, &batchOptions);
}
