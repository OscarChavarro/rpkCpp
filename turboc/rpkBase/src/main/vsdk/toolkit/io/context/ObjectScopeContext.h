#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __OBJECT_SCOPE_CONTEXT__
#define __OBJECT_SCOPE_CONTEXT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/io/context/ParseErrorContext.h"

class ObjectScopeContext {
  private:
    ArrayList<char *> objectNamesList;

  public:
    ObjectScopeContext();
    ~ObjectScopeContext();

    int pushName(const char *name);
    int popName();
    void clear();

    ObjectScopeContext(const ObjectScopeContext &);
    ObjectScopeContext &operator=(const ObjectScopeContext &);
};

#endif
