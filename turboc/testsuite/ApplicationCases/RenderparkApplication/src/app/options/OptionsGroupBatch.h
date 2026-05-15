#ifndef CMMND_LINE_BTCH_OPTNS_GRP
#define CMMND_LINE_BTCH_OPTNS_GRP

#include "app/options/BatchOptions.h"

class OptionsGroupBatch{ public:
    static void parse( int *argc, char **argv, BatchOptions &batchOptions);

    static void batchParseOptions( int *argc, char **argv, BatchOptions *options);

  private:
    static BatchOptions batchOptionsState;

    static void binaryOutputOption(const char *&value);
    static void binaryInputOption(const char *&value);
    static void setIntTrue(int &value);
};

#endif
