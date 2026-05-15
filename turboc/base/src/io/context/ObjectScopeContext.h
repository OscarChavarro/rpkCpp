#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

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

    ObjectScopeContext(const ObjectScopeContext &);
    ObjectScopeContext &operator=(const ObjectScopeContext &);

  private:
    #define OBJECT_NAMES_ALLOC_INCREMENT 16
};

#endif
