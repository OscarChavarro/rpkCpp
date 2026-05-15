#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __READER_CONTEXT__
#define __READER_CONTEXT__

#include "java/io/InputStream.h"

class ReaderContext {
  public:
    enum{
        MGF_MAXIMUM_INPUT_LINE_LENGTH = 4096,
        MGF_MAXIMUM_ARGUMENT_COUNT = (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4)
    };

    char fileName[96];
    InputStream *inputStream; // stream pointer
    int fileContextId;
    char inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    int lineNumber;
    char isPipe; // Flag indicating whether input comes from a pipe or a real file
    ReaderContext *prev; // Previous context
};

#endif
