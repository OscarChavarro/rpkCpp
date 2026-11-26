#include <cstring>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/io/context/ObjectScopeContext.h"

ObjectScopeContext::ObjectScopeContext(): objectNamesList() {
}

ObjectScopeContext::~ObjectScopeContext() {
    clear();
}

int
ObjectScopeContext::pushName(const char *name) {
    if ( name == nullptr ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    char *nameCopy = new char[std::strlen(name) + 1];
    std::strcpy(nameCopy, name);
    if ( !objectNamesList.add(nameCopy) ) {
        delete[] nameCopy;
        return ParseErrorContext::MGF_ERROR_OUT_OF_MEMORY;
    }
    return ParseErrorContext::MGF_OK;
}

int
ObjectScopeContext::popName() {
    const long int count = objectNamesList.size();
    if ( count < 1 ) {
        return ParseErrorContext::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
    }
    delete[] objectNamesList.get(count - 1);
    objectNamesList.remove(count - 1);
    return ParseErrorContext::MGF_OK;
}

void
ObjectScopeContext::clear() {
    for ( long int i = 0; i < objectNamesList.size(); i++ ) {
        delete[] objectNamesList.get(i);
    }
    objectNamesList.clear();
}
