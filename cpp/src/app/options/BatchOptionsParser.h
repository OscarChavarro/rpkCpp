#ifndef __BATCH_OPTIONS_PARSER__
#define __BATCH_OPTIONS_PARSER__

#include "app/options/BatchOptions.h"
#include "app/options/OptionsType.h"

class BatchOptionsParser final {
  public:
    static void parse(
        int *argc,
        char **argv,
        BatchOptions &batchOptions,
        OptionsType &optionTypes);
};

#endif
