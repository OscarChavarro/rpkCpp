#ifndef __LOOKUP_TABLE__
#define __LOOKUP_TABLE__

class LookUpEntity;

class LookUpTable {
  public:
    LookUpTable();
    LookUpTable(
        void (*freeKeyFunction)(const char *),
        void (*freeDataFunction)(const char *));

    static void
    lookUpRemove(const char *data);

    long
    (*getKeyHashFunction() const)(const char *);

    int
    (*getKeyCompareFunction() const)(const char *, const char *);

    void
    (*getFreeKeyFunction() const)(const char *);

    void
    (*getFreeDataFunction() const)(const char *);

    int
    getCurrentTableSize() const;

    LookUpEntity *
    getTable() const;

    int
    getNumberOfDeletedEntries() const;

    void
    setCurrentTableSize(int value);

    void
    setTable(LookUpEntity *value);

    void
    setNumberOfDeletedEntries(int value);

    int
    lookUpInit(int nel);

    LookUpEntity *
    lookUpFind(const char *key);

    void
    lookUpDone();

  private:
    static long
    lookUpShuffleHash(const char *s);

    long (*keyHashFunction)(const char *);
    int (*keyCompareFunction)(const char *, const char *);
    void (*freeKeyFunction)(const char *);
    void (*freeDataFunction)(const char *);
    int currentTableSize;
    LookUpEntity *table;
    int numberOfDeletedEntries;
};

#endif
