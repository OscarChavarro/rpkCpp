#include <string.h>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/io/context/ObjectScopeContext.h"

ObjectScopeContext::ObjectScopeContext(): objectNamesList() {
}

ObjectScopeContext::~ObjectScopeContext() {
    clear();
}

int
ObjectScopeContext::pushName(const char *name) {
    if ( name == NULL ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    char *nameCopy = new char[strlen(name) + 1];
    strcpy(nameCopy, name);
    if ( !objectNamesList.add(nameCopy) ) {
        delete[] nameCopy;
        return MGF_ERROR_OUT_OF_MEMORY;
    }
    return MGF_OK;
}

int
ObjectScopeContext::popName() {
    const long int count = objectNamesList.size();
    if ( count < 1 ) {
        return MGF_ERRR_UNMTC_CNTXT_CLS;
    }
    delete[] objectNamesList.get(count - 1);
    objectNamesList.remove(count - 1);
    return MGF_OK;
}

void
ObjectScopeContext::clear() {
    for ( long int i = 0; i < objectNamesList.size(); i++ ) {
        delete[] objectNamesList.get(i);
    }
    objectNamesList.clear();
}
