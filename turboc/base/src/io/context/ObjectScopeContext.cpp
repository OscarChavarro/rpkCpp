#include <string.h>

#include "common/memoryManagement/CppReAlloc.h"
#include "io/context/ObjectScopeContext.h"

ObjectScopeContext::ObjectScopeContext():
    objectNamesList(NULL),
    objectMaxName(0),
    objectNames(0)
{
}

ObjectScopeContext::~ObjectScopeContext() {
    clear();
}

int
ObjectScopeContext::pushName(const char *name) {
    if ( name == NULL ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
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
        if ( objectNamesList == NULL ) {
            objectMaxName = 0;
            objectNames = 0;
            return MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    objectNamesList[objectNames] = new char[strlen(name) + 1];
    if ( objectNamesList[objectNames] == NULL ) {
        return MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(objectNamesList[objectNames++], name);
    objectNamesList[objectNames] = NULL;
    return MGF_OK;
}

int
ObjectScopeContext::popName() {
    if ( objectNames < 1 ) {
        return MGF_ERRR_UNMTC_CNTXT_CLS;
    }
    delete[] objectNamesList[--objectNames];
    objectNamesList[objectNames] = NULL;
    return MGF_OK;
}

void
ObjectScopeContext::clear() {
    for ( int i = 0; i < objectNames; i++ ) {
        delete[] objectNamesList[i];
    }
    delete[] objectNamesList;
    objectNamesList = NULL;
    objectMaxName = 0;
    objectNames = 0;
}
