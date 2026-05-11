#ifndef COMMAND_LINE_BATCH_OPTIONS_GROUP__
#define COMMAND_LINE_BATCH_OPTIONS_GROUP__

#include "options/BatchOptions.h"

class OptionsGroupBatch final {
  public:
    static void parse(
        int *argc,
        char **argv,
        BatchOptions &batchOptions);

    static void batchParseOptions(
        int *argc,
        char **argv,
        BatchOptions *options);

  private:
    static BatchOptions batchOptionsState;

    static void binaryOutputOption(const char *&value);
    static void binaryInputOption(const char *&value);
    static void setIntTrue(int &value);
};

#endif
