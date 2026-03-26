#ifndef __LOOKUP__
#define __LOOKUP__

/**
General associative table lookup routines
*/

class LookUpEntity {
  public:
    char *key; // Key name
    long value; // Key hash value (for efficiency)
    char *data; // Client data
};

class LookUpTable {
  public:
    long (*keyHashFunction)(const char *);
    int (*keyCompareFunction)(const char *, const char *);
    void (*freeKeyFunction)(const char *);
    void (*freeDataFunction)(const char *);
    int currentTableSize;
    LookUpEntity *table;
    int numberOfDeletedEntries;
};

#define LOOK_UP_INIT(fk, fd) { \
    lookUpShuffleHash, \
    strcmp, \
    (fk), \
    (fd), \
    0, \
    nullptr, \
    0 \
}

extern void lookUpRemove(const char *data);
extern int lookUpInit(LookUpTable *tbl, int nel);
extern LookUpEntity *lookUpFind(LookUpTable *tbl, const char *key);
extern void lookUpDone(LookUpTable *l);
extern long lookUpShuffleHash(const char *s);

#endif
