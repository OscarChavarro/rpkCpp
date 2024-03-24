/**
Header file for general associative table lookup routines
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

extern LookUpTable GLOBAL_mgf_vertexLookUpTable;

#define LOOK_UP_INIT(fk, fd) { \
    (long (*)(char *))lookUpSHash, \
    (int (*)(const char *, const char *))strcmp, \
    (void (*)(char *))(fk), \
    (void (*)(char *))(fd), \
    0, \
    nullptr, \
    0 \
}

/**
The lookUpInit routine is called to initialize a table.  The number of
elements passed is not a limiting factor, as a table can grow to
any size permitted by memory.  However, access will be more efficient
if this number strikes a reasonable balance between default memory use
and the expected (minimum) table size.  The value returned is the
actual allocated table size (or zero if there was insufficient memory).
*/
extern int lookUpInit(LookUpTable *tbl, int nel);

/**
The lookUpFind routine returns the entry corresponding to the given
key.  If the entry does not exist, the corresponding key field will
be nullptr.  If the entry has been previously deleted but not yet freeDataFunction,
then only the data field will be nullptr.  It is the caller's
responsibility to (allocate and) assign the key and data fields when
creating a new entry.  The only case where lookUpFind returns nullptr is when
the system has run out of memory.
*/
extern LookUpEntity *lookUpFind(LookUpTable *tbl, char *key);

/**
The lookUpDone routine calls the given free function once for each
assigned table entry (i.e. each entry with an assigned key value).
The user must define these routines to free the key and the data
in the LU_TAB structure.  The final action of lookUpDone is to free the
allocated table itself.
*/
extern void lookUpDone(LookUpTable *l);

/**
The functions must be assigned separately.  If the hash value is sufficient to
guarantee equality between keys, then the keyCompareFunction pointer may be nullptr.  Otherwise, it
should return 0 if the two passed keys match.  If it is not necessary
(or possible) to free the key and/or data values, then the freeKeyFunction and/or
freeDataFunction member functions may be nullptr.

It isn't fully necessary to call lookUpInit to initialize the LookUpTable structure.
If currentTableSize is 0, then the first call to lookUpFind will allocate a minimal table.
The LOOK_UP_INIT macro provides a convenient static declaration for character
string keys.
*/
extern long lookUpSHash(char *s);
