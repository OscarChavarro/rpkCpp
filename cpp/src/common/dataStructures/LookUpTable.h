#ifndef COMMON_LOOKUP_TABLE__
#define COMMON_LOOKUP_TABLE__

#include "common/dataStructures/LookUpBehaviors.h"
#include "common/dataStructures/LookUpEntity.h"

template<typename T>
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

    LookUpEntity<T> *
    lookUpFind(const char *key);

    void
    lookUpDone();

  private:
    static long lookUpShuffleHash(const char *text);
    bool keysEqual(const char *left, const char *right) const;
    void freeKey(const char *key) const;
    void freeData(T data) const;

    int
    lookUpReAlloc(int nel);

    LookUpBehaviors behaviorType;
    int currentTableSize;
    LookUpEntity<T> *table;
    int numberOfDeletedEntries;
};

#include "common/dataStructures/LookUpTable.txx"

#endif
