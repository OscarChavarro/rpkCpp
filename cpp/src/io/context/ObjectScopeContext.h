#ifndef __OBJECT_HIERARCHY_STATE__
#define __OBJECT_HIERARCHY_STATE__

#include "io/context/ParseErrorContext.h"

class ObjectScopeContext {
  public:
    char **objectNamesList;
    int objectMaxName;
    int objectNames;

    ObjectScopeContext();
    ~ObjectScopeContext();

    int pushName(const char *name);
    int popName();
    void clear();

    ObjectScopeContext(const ObjectScopeContext &) = delete;
    ObjectScopeContext &operator=(const ObjectScopeContext &) = delete;

  private:
    static constexpr int OBJECT_NAMES_ALLOC_INCREMENT = 16;
};

#endif
