#ifndef __LOOKUP_TABLE__
#define __LOOKUP_TABLE__

#include "io/context/LookUpBehaviors.h"
#include "io/context/LookUpEntity.h"

class LookUpTable {
  public:
    LookUpTable();
    explicit LookUpTable(LookUpBehaviors behaviorType);
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
    static long lookUpShuffleHash(const char *text);
    bool keysEqual(const char *left, const char *right) const;
    void freeKey(const char *key) const;
    void freeData(const char *data) const;

    int
    lookUpReAlloc(int nel);

    LookUpBehaviors behaviorType;
    int currentTableSize;
    LookUpEntity *table;
    int numberOfDeletedEntries;
};

#endif
