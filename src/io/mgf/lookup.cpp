/**
Table lookup routines
*/

#include <cstdlib>

#include "io/mgf/lookup.h"

static unsigned char globalShuffle[256] = {
    0, 157, 58, 215, 116, 17, 174, 75, 232, 133, 34, 191, 92, 249, 150, 51,
    208, 109, 10, 167, 68, 225, 126, 27, 184, 85, 242, 143, 44, 201, 102, 3,
    160, 61, 218, 119, 20, 177, 78, 235, 136, 37, 194, 95, 252, 153, 54, 211,
    112, 13, 170, 71, 228, 129, 30, 187, 88, 245, 146, 47, 204, 105, 6, 163,
    64, 221, 122, 23, 180, 81, 238, 139, 40, 197, 98, 255, 156, 57, 214, 115,
    16, 173, 74, 231, 132, 33, 190, 91, 248, 149, 50, 207, 108, 9, 166, 67,
    224, 125, 26, 183, 84, 241, 142, 43, 200, 101, 2, 159, 60, 217, 118, 19,
    176, 77, 234, 135, 36, 193, 94, 251, 152, 53, 210, 111, 12, 169, 70, 227,
    128, 29, 186, 87, 244, 145, 46, 203, 104, 5, 162, 63, 220, 121, 22, 179,
    80, 237, 138, 39, 196, 97, 254, 155, 56, 213, 114, 15, 172, 73, 230, 131,
    32, 189, 90, 247, 148, 49, 206, 107, 8, 165, 66, 223, 124, 25, 182, 83,
    240, 141, 42, 199, 100, 1, 158, 59, 216, 117, 18, 175, 76, 233, 134, 35,
    192, 93, 250, 151, 52, 209, 110, 11, 168, 69, 226, 127, 28, 185, 86, 243,
    144, 45, 202, 103, 4, 161, 62, 219, 120, 21, 178, 79, 236, 137, 38, 195,
    96, 253, 154, 55, 212, 113, 14, 171, 72, 229, 130, 31, 188, 89, 246, 147,
    48, 205, 106, 7, 164, 65, 222, 123, 24, 181, 82, 239, 140, 41, 198, 99
};

/**
Initialize tbl for at least nel elements

The lookUpInit routine is called to initialize a table.  The number of
elements passed is not a limiting factor, as a table can grow to
any size permitted by memory.  However, access will be more efficient
if this number strikes a reasonable balance between default memory use
and the expected (minimum) table size.  The value returned is the
actual allocated table size (or zero if there was insufficient memory).
*/
int
lookUpInit(LookUpTable *tbl, int nel) {
    static int hSizeTab[] = {
            31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
            32749, 65521, 131071, 262139, 524287, 1048573, 2097143,
            4194301, 8388593, 0
    };
    const int *hsp;

    nel += nel >> 1; // 66% occupancy
    for ( hsp = hSizeTab; *hsp; hsp++ ) {
        if ( *hsp > nel ) {
            break;
        }
    }
    if ( !(tbl->currentTableSize = *hsp) ) {
        // Not always prime
        tbl->currentTableSize = nel * 2 + 1;
    }
    tbl->table = new LookUpEntity[tbl->currentTableSize];
    for ( int i = 0; i < tbl->currentTableSize; i++ ) {
        tbl->table[i].key = nullptr;
        tbl->table[i].data = nullptr;
        tbl->table[i].value = 0;
    }
    if ( tbl->table == nullptr ) {
        tbl->currentTableSize = 0;
    }
    tbl->numberOfDeletedEntries = 0;

    return tbl->currentTableSize;
}

/**
Hash a null-terminated string

The functions must be assigned separately.  If the hash value is sufficient to
guarantee equality between keys, then the keyCompareFunction pointer may be nullptr.  Otherwise, it
should return 0 if the two passed keys match.  If it is not necessary
(or possible) to free the key and/or data values, then the freeKeyFunction and/or
freeDataFunction member functions may be nullptr.

It isn't fully necessary to call lookUpInit to initialize the LookUpTable structure.
If currentTableSize is 0, then the first call to lookUpFind will allocate a minimal table.
The LOOK_UP_INIT macro provides a convenient static declaration for character
string keys
*/
long
lookUpShuffleHash(char *s) {
    int i = 0;
    long h = 0;
    const unsigned char *t = (unsigned char *)s;

    while ( *t ) {
        h ^= (long)globalShuffle[*t++] << ((i += 11) & 0xf);
    }

    return h;
}

int
lookUpReAlloc(LookUpTable *tbl, int nel) {
    const LookUpEntity *le;
    int oldTSize;
    LookUpEntity *oldTable;

    oldTable = tbl->table;
    oldTSize = tbl->currentTableSize;
    int i = tbl->numberOfDeletedEntries;
    if ( !lookUpInit(tbl, nel) ) {
        // No more memory!
        tbl->table = oldTable;
        tbl->currentTableSize = oldTSize;
        tbl->numberOfDeletedEntries = i;
        return 0;
    }

    // The following code may fail if the user has reclaimed many
    // deleted entries and the system runs out of memory in a
    // recursive call to lu_find()
    for ( i = 0, le = oldTable; i < oldTSize; i++, le++ ) {
        if ( le->key != nullptr) {
            if ( le->data != nullptr) {
                *lookUpFind(tbl, le->key) = *le;
            } else {
                if ( tbl->freeKeyFunction != nullptr) {
                    (*tbl->freeKeyFunction)(le->key);
                }
            }
        }
    }
    delete[] oldTable;

    return tbl->currentTableSize;
}

/**
Find a table entry

The lookUpFind routine returns the entry corresponding to the given
key.  If the entry does not exist, the corresponding key field will
be nullptr.  If the entry has been previously deleted but not yet freeDataFunction,
then only the data field will be nullptr.  It is the caller's
responsibility to (allocate and) assign the key and data fields when
creating a new entry.  The only case where lookUpFind returns nullptr is when
the system has run out of memory.
*/
LookUpEntity *
lookUpFind(LookUpTable *tbl, const char *key) {
    long hVal;
    int ndx;
    int i;
    int n;
    LookUpEntity *le;

    // Look up object
    if ( tbl->currentTableSize <= 0 ) {
        lookUpInit(tbl, 1);
    }

    hVal = (*tbl->keyHashFunction)(key);

    do {
        ndx = (int)(hVal % tbl->currentTableSize);
        le = &tbl->table[ndx];
        i = 0;
        n = -1;
        do {
            if ( le->key == nullptr ) {
                le->value = hVal;
                return le;
            }
            if ( le->value == hVal &&
                 (tbl->keyCompareFunction == nullptr || (*tbl->keyCompareFunction)(le->key, key) == 0) ) {
                return le;
            }

            i++;
            n += 2;
            le += n;
            if ( (ndx += n) >= tbl->currentTableSize ) {
                // This happens rarely
                ndx = ndx % tbl->currentTableSize;
                le = &tbl->table[ndx];
            }
        }
        while ( i < tbl->currentTableSize );

        if ( !lookUpReAlloc(tbl, tbl->currentTableSize - tbl->numberOfDeletedEntries + 1) ) {
            // Table is full, reallocate
            return nullptr;
        }
    } while ( true ); // Should happen only once!
}

/**
Free table and contents

The lookUpDone routine calls the given free function once for each
assigned table entry (i.e. each entry with an assigned key value).
The user must define these routines to free the key and the data
in the LU_TAB structure.  The final action of lookUpDone is to free the
allocated table itself.
*/
void
lookUpDone(LookUpTable *l) {
    if ( !l->currentTableSize ) {
        return;
    }

    for ( const LookUpEntity *tp = l->table + l->currentTableSize; tp-- > l->table; ) {
        if ( tp->key != nullptr) {
            if ( l->freeKeyFunction != nullptr) {
                (*l->freeKeyFunction)(tp->key);
            }
            if ( tp->data != nullptr && l->freeDataFunction != nullptr) {
                (*l->freeDataFunction)(tp->data);
            }
        }
    }
    delete[] l->table;
    l->table = nullptr;
    l->currentTableSize = 0;
    l->numberOfDeletedEntries = 0;
}
