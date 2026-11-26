#ifndef __OBJECT_SCOPE_CONTEXT__
#define __OBJECT_SCOPE_CONTEXT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/io/context/ParseErrorContext.h"

class ObjectScopeContext {
  private:
    java::ArrayList<char *> objectNamesList;

  public:
    ObjectScopeContext();
    ~ObjectScopeContext();

    int pushName(const char *name);
    int popName();
    void clear();

    ObjectScopeContext(const ObjectScopeContext &) = delete;
    ObjectScopeContext &operator=(const ObjectScopeContext &) = delete;
};

#endif
