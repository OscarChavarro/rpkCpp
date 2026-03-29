#ifndef __READER_STACK_STATE__
#define __READER_STACK_STATE__

#include "io/context/EntityContext.h"
#include "io/context/ErrorCodeContext.h"
#include "io/context/ReaderContext.h"

class MgfEntityHandler;

class ReaderStackState {
  public:
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[ErrorCodeContext::MGF_NUMBER_OF_ERRORS];
    ReaderContext *readerContext;
    MgfEntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    MgfEntityHandler *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];

    ReaderStackState();
};

#endif
