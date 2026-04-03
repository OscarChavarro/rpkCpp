#ifndef __READER_STACK_STATE__
#define __READER_STACK_STATE__

#include "io/context/EntityContext.h"
#include "io/context/EntityContextInfo.h"
#include "io/context/EntityHandler.h"
#include "io/context/ErrorCodeContext.h"
#include "io/context/LookUpTable.h"
#include "io/context/ReaderContext.h"
#include "io/context/HandlerType.h"

class ReaderStackState {
  private:
    static constexpr int TOTAL_MGF_HANDLER_TYPES = static_cast<int>(HandlerType::HANDLE_OBJECT) + 1;

  public:
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][EntityContextInfo::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[ErrorCodeContext::MGF_NUMBER_OF_ERRORS];
    LookUpTable<char *> entityLookUpTable;
    int nextFileContextId;
    ReaderContext *readerContext;
    EntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityHandler *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityHandler *handlerByType[TOTAL_MGF_HANDLER_TYPES];

    static constexpr int handlerTypeCount() {
        return TOTAL_MGF_HANDLER_TYPES;
    }

    ReaderStackState();
    ~ReaderStackState();
};

#endif
