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
    long (*keyHashFunction)(char *);
    int (*keyCompareFunction)(const char *, const char *);
    void (*freeKeyFunction)(char *);
    void (*freeDataFunction)(char *);
    int currentTableSize;
    LookUpEntity *table;
    int numberOfDeletedEntries;
};

#define LOOK_UP_INIT(fk, fd) { \
    (long (*)(char *))lookUpShuffleHash, \
    (int (*)(const char *, const char *))strcmp, \
    (void (*)(char *))(fk), \
    (void (*)(char *))(fd), \
    0, \
    nullptr, \
    0 \
}

extern int lookUpInit(LookUpTable *tbl, int nel);
extern LookUpEntity *lookUpFind(LookUpTable *tbl, char *key);
extern void lookUpDone(LookUpTable *l);
extern long lookUpShuffleHash(char *s);

#endif
