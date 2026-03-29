#ifndef __LOOKUP_TABLE__
#define __LOOKUP_TABLE__

#include "io/context/LookUpBehavior.h"
#include "io/context/CStringLookUpBehavior.h"
#include "io/context/OwningCStringLookUpBehavior.h"

class LookUpEntity;

namespace LookUpBehaviors {
const LookUpBehavior &
nonOwningCString();

const LookUpBehavior &
owningCString();
}

class LookUpTable {
  public:
    LookUpTable();
    explicit LookUpTable(const LookUpBehavior &behavior);
    ~LookUpTable();

    LookUpTable(const LookUpTable &) = delete;
    LookUpTable &operator=(const LookUpTable &) = delete;

    int
    getCurrentTableSize() const;

    int
    lookUpInit(int nel);

    LookUpEntity *
    lookUpFind(const char *key);

    void
    lookUpDone();

  private:
    int
    lookUpReAlloc(int nel);

    const LookUpBehavior &behavior;
    int currentTableSize;
    LookUpEntity *table;
    int numberOfDeletedEntries;
};

#endif
