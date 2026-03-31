#include <cstring>

#include "io/context/ReaderStackState.h"
#include "io/context/EntityHandler.h"

ReaderStackState::ReaderStackState():
    entityNames(),
    errorCodeMessages(),
    entityLookUpTable(LookUpBehaviors::NON_OWNING),
    nextFileContextId(0),
    readerContext(nullptr),
    handleCallbacks(),
    supportCallbacks(),
    handlerByType()
{
    std::strcpy(entityNames[0], "#");
    std::strcpy(entityNames[1], "c");
    std::strcpy(entityNames[2], "cct");
    std::strcpy(entityNames[3], "cone");
    std::strcpy(entityNames[4], "cmix");
    std::strcpy(entityNames[5], "cspec");
    std::strcpy(entityNames[6], "cxy");
    std::strcpy(entityNames[7], "cyl");
    std::strcpy(entityNames[8], "ed");
    std::strcpy(entityNames[9], "f");
    std::strcpy(entityNames[10], "i");
    std::strcpy(entityNames[11], "ies");
    std::strcpy(entityNames[12], "ir");
    std::strcpy(entityNames[13], "m");
    std::strcpy(entityNames[14], "n");
    std::strcpy(entityNames[15], "o");
    std::strcpy(entityNames[16], "p");
    std::strcpy(entityNames[17], "prism");
    std::strcpy(entityNames[18], "rd");
    std::strcpy(entityNames[19], "ring");
    std::strcpy(entityNames[20], "rs");
    std::strcpy(entityNames[21], "sides");
    std::strcpy(entityNames[22], "sph");
    std::strcpy(entityNames[23], "td");
    std::strcpy(entityNames[24], "torus");
    std::strcpy(entityNames[25], "ts");
    std::strcpy(entityNames[26], "v");
    std::strcpy(entityNames[27], "xf");
    std::strcpy(entityNames[28], "fh");

    errorCodeMessages[0] = "No error";
    errorCodeMessages[1] = "Unknown entity";
    errorCodeMessages[2] = "Wrong number of arguments";
    errorCodeMessages[3] = "Wrong argument type";
    errorCodeMessages[4] = "Illegal argument value";
    errorCodeMessages[5] = "Undefined reference";
    errorCodeMessages[6] = "Cannot open input file";
    errorCodeMessages[7] = "Error in included file";
    errorCodeMessages[8] = "Out of memory";
    errorCodeMessages[9] = "Seek failure";
    errorCodeMessages[10] = "Illegal material specification";
    errorCodeMessages[11] = "Input line too long";
    errorCodeMessages[12] = "Unmatched context close";
}

ReaderStackState::~ReaderStackState() {
    for ( int i = 0; i < TOTAL_MGF_HANDLER_TYPES; i++ ) {
        if ( handlerByType[i] != nullptr ) {
            delete handlerByType[i];
            handlerByType[i] = nullptr;
        }
    }
}
