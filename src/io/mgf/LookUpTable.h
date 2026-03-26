#ifndef __LOOKUP_TABLE__
#define __LOOKUP_TABLE__

class LookUpEntity;

class LookUpBehavior {
  public:
    virtual ~LookUpBehavior() {}

    virtual long
    hash(const char *key) const = 0;

    virtual bool
    keysEqual(const char *left, const char *right) const = 0;

    virtual void
    freeKey(const char * /*key*/) const {}

    virtual void
    freeData(const char * /*data*/) const {}
};

class CStringLookUpBehavior : public LookUpBehavior {
  public:
    long
    hash(const char *key) const override;

    bool
    keysEqual(const char *left, const char *right) const override;
};

class OwningCStringLookUpBehavior : public CStringLookUpBehavior {
  public:
    void
    freeKey(const char *key) const override;

    void
    freeData(const char *data) const override;
};

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
