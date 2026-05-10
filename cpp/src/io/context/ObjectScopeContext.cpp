#include <cstring>

#include "common/memoryManagement/CppReAlloc.h"
#include "io/context/ObjectScopeContext.h"

ObjectScopeContext::ObjectScopeContext():
    objectNamesList(nullptr),
    objectMaxName(0),
    objectNames(0)
{
}

ObjectScopeContext::~ObjectScopeContext() {
    clear();
}

int
ObjectScopeContext::pushName(const char *name) {
    if ( name == nullptr ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( objectNames >= objectMaxName - 1 ) {
        if ( objectMaxName == 0 ) {
            objectMaxName = OBJECT_NAMES_ALLOC_INCREMENT;
            objectNamesList = new char *[objectMaxName];
        } else {
            const int previousMaxName = objectMaxName;
            objectMaxName += OBJECT_NAMES_ALLOC_INCREMENT;
            objectNamesList = CppReAlloc::reAlloc(
                objectNamesList,
                previousMaxName,
                objectMaxName);
        }
        if ( objectNamesList == nullptr ) {
            objectMaxName = 0;
            objectNames = 0;
            return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    objectNamesList[objectNames] = new char[std::strlen(name) + 1];
    if ( objectNamesList[objectNames] == nullptr ) {
        return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
    }
    std::strcpy(objectNamesList[objectNames++], name);
    objectNamesList[objectNames] = nullptr;
    return ParseErrorContext::MGF_OK;
}

int
ObjectScopeContext::popName() {
    if ( objectNames < 1 ) {
        return ParseErrorContext::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
    }
    delete[] objectNamesList[--objectNames];
    objectNamesList[objectNames] = nullptr;
    return ParseErrorContext::MGF_OK;
}

void
ObjectScopeContext::clear() {
    for ( int i = 0; i < objectNames; i++ ) {
        delete[] objectNamesList[i];
    }
    delete[] objectNamesList;
    objectNamesList = nullptr;
    objectMaxName = 0;
    objectNames = 0;
}
