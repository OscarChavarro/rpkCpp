#ifndef __BATCH_OPTIONS_GROUP__
#define __BATCH_OPTIONS_GROUP__

#include "app/options/BatchOptions.h"

class BatchOptionsGroup final {
  public:
    static void parse(
        int *argc,
        char **argv,
        BatchOptions &batchOptions);
};

#endif
