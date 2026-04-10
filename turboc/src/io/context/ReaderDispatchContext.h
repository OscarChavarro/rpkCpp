#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __READER_STACK_STATE__
#define __READER_STACK_STATE__

#include "io/context/EntityTypeContext.h"
#include "io/context/EntityNamingContext.h"
#include "io/context/EntityDispatchContext.h"
#include "io/context/ParseErrorContext.h"
#include "common/dataStructures/LookUpTable.h"
#include "io/context/ReaderContext.h"
#include "io/context/HandlerRoleContext.h"

class ReaderDispatchContext {
  private:
    enum{
        TOTAL_MGF_HANDLER_TYPES = ((int)(HANDLE_OBJECT)) + 1
    };

  public:
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][EntityNamingContext::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[MGF_NUMBER_OF_ERRORS];
    LookUpTable<char *> entityLookUpTable;
    int nextFileContextId;
    ReaderContext *readerContext;
    EntityDispatchContext *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityDispatchContext *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    EntityDispatchContext *handlerByType[TOTAL_MGF_HANDLER_TYPES];

    static int handlerTypeCount() {
        return TOTAL_MGF_HANDLER_TYPES;
    }

    ReaderDispatchContext();
    ~ReaderDispatchContext();
};

#endif
