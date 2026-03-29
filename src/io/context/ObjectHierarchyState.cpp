#include <cstring>

#include "common/CppReAlloc.h"
#include "io/context/ObjectHierarchyState.h"

namespace {
constexpr int OBJECT_NAMES_ALLOC_INCREMENT = 16;
}

ObjectHierarchyState::ObjectHierarchyState():
    objectNamesList(nullptr),
    objectMaxName(0),
    objectNames(0)
{
}

ObjectHierarchyState::~ObjectHierarchyState() {
    clear();
}

int
ObjectHierarchyState::pushName(const char *name) {
    if ( name == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
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
            return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    objectNamesList[objectNames] = new char[std::strlen(name) + 1];
    if ( objectNamesList[objectNames] == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
    }
    std::strcpy(objectNamesList[objectNames++], name);
    objectNamesList[objectNames] = nullptr;
    return ErrorCodeContext::MGF_OK;
}

int
ObjectHierarchyState::popName() {
    if ( objectNames < 1 ) {
        return ErrorCodeContext::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
    }
    delete[] objectNamesList[--objectNames];
    objectNamesList[objectNames] = nullptr;
    return ErrorCodeContext::MGF_OK;
}

void
ObjectHierarchyState::clear() {
    for ( int i = 0; i < objectNames; i++ ) {
        delete[] objectNamesList[i];
    }
    delete[] objectNamesList;
    objectNamesList = nullptr;
    objectMaxName = 0;
    objectNames = 0;
}
