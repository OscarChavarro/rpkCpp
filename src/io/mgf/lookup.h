/*
Header file for general associative table lookup routines
*/

class LUENT {
  public:
    char *key; // key name
    long hval; // key hash value (for efficiency)
    char *data; // pointer to client data
};

class LUTAB {
  public:
    long (*hashf)(char *); // key hash function
    int (*keycmp)(const char *, const char *); // key comparison function
    void (*freek)(char *); // free a key
    void (*freed)(char *); // free the data
    int tsiz; // current table size
    LUENT *tabl; // table, if allocated
    int ndel; // number of deleted entries
};

#define LU_SINIT(fk, fd) { \
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
extern int lookUpInit(LUTAB *tbl, int nel);

/**
The lookUpFind routine returns the entry corresponding to the given
key.  If the entry does not exist, the corresponding key field will
be nullptr.  If the entry has been previously deleted but not yet freed,
then only the data field will be nullptr.  It is the caller's
responsibility to (allocate and) assign the key and data fields when
creating a new entry.  The only case where lookUpFind returns nullptr is when
the system has run out of memory.
*/
extern LUENT *lookUpFind(LUTAB *tbl, char *key);

/**
The lookUpDone routine calls the given free function once for each
assigned table entry (i.e. each entry with an assigned key value).
The user must define these routines to free the key and the data
in the LU_TAB structure.  The final action of lookUpDone is to free the
allocated table itself.
*/
extern void lookUpDone(LUTAB *l);

/**
The hashf, keycmp, freek and freed member functions must be assigned
separately.  If the hash value is sufficient to guarantee equality
between keys, then the keycmp pointer may be nullptr.  Otherwise, it
should return 0 if the two passed keys match.  If it is not necessary
(or possible) to free the key and/or data values, then the freek and/or
freed member functions may be nullptr.

It isn't fully necessary to call lookUpInit to initialize the LUTAB structure.
If tsiz is 0, then the first call to lookUpFind will allocate a minimal table.
The LU_SINIT macro provides a convenient static declaration for character
string keys.
*/
extern long lookUpSHash(char *s);
