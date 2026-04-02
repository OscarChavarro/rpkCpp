#ifndef __OBJECT_HIERARCHY_STATE__
#define __OBJECT_HIERARCHY_STATE__

#include "io/context/ErrorCodeContext.h"

class ObjectHierarchyState {
  public:
    char **objectNamesList;
    int objectMaxName;
    int objectNames;

    ObjectHierarchyState();
    ~ObjectHierarchyState();

    int pushName(const char *name);
    int popName();
    void clear();

    ObjectHierarchyState(const ObjectHierarchyState &) = delete;
    ObjectHierarchyState &operator=(const ObjectHierarchyState &) = delete;

  private:
    static constexpr int OBJECT_NAMES_ALLOC_INCREMENT = 16;
};

#endif
