#ifndef __READER_STACK_STATE__
#define __READER_STACK_STATE__

#include "io/context/EntityContext.h"
#include "io/context/ErrorCodeContext.h"
#include "io/context/LookUpTable.h"
#include "io/context/ReaderContext.h"
#include "io/mgf/MgfHandlerType.h"

class MgfEntityHandler;

constexpr int TOTAL_MGF_HANDLER_TYPES = static_cast<int>(MgfHandlerType::HANDLE_OBJECT) + 1;

class ReaderStackState {
  public:
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[ErrorCodeContext::MGF_NUMBER_OF_ERRORS];
    LookUpTable entityLookUpTable;
    int nextFileContextId;
    ReaderContext *readerContext;
    MgfEntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    MgfEntityHandler *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    MgfEntityHandler *handlerByType[TOTAL_MGF_HANDLER_TYPES];

    ReaderStackState();
    ~ReaderStackState();
};

#endif
