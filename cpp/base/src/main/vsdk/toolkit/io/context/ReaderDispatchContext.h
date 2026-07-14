#ifndef READER_STACK_STATE__
#define READER_STACK_STATE__

#include "vsdk/toolkit/io/context/EntityTypeContext.h"
#include "vsdk/toolkit/io/context/EntityNamingContext.h"
#include "vsdk/toolkit/io/context/EntityDispatchContext.h"
#include "vsdk/toolkit/io/context/ParseErrorContext.h"
#include "vsdk/toolkit/common/dataStructures/LookUpTable.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/HandlerRoleContext.h"

class ReaderDispatchContext {
  private:
    static constexpr int TOTAL_MGF_HANDLER_TYPES = static_cast<int>(HandlerRoleContext::HANDLE_OBJECT) + 1;

  public:
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][EntityNamingContext::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[ParseErrorContext::MGF_NUMBER_OF_ERRORS];
    LookUpTable<char *> entityLookUpTable;
    int nextFileContextId;
    ReaderContext *readerContext;
    EntityDispatchContext *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityDispatchContext *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityDispatchContext *handlerByType[TOTAL_MGF_HANDLER_TYPES];

    static constexpr int handlerTypeCount() {
        return TOTAL_MGF_HANDLER_TYPES;
    }

    ReaderDispatchContext();
    ~ReaderDispatchContext();
};

#endif
