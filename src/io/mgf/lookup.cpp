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
*/
int
lookUpInit(LUTAB *tbl, int nel)
{
    static int hSizeTab[] = {
            31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
            32749, 65521, 131071, 262139, 524287, 1048573, 2097143,
            4194301, 8388593, 0
    };
    int *hsp;

    nel += nel >> 1; // 66% occupancy
    for ( hsp = hSizeTab; *hsp; hsp++ ) {
        if ( *hsp > nel ) {
            break;
        }
    }
    if ( !(tbl->tsiz = *hsp)) {
        tbl->tsiz = nel * 2 + 1;
    }        /* not always prime */
    tbl->tabl = (LUENT *) calloc(tbl->tsiz, sizeof(LUENT));
    if ( tbl->tabl == nullptr) {
        tbl->tsiz = 0;
    }
    tbl->ndel = 0;

    return tbl->tsiz;
}

/**
Hash a nul-terminated string
*/
long
lookUpSHash(char *s)
{
    int i = 0;
    long h = 0;
    unsigned char *t = (unsigned char *)s;

    while ( *t ) {
        h ^= (long) globalShuffle[*t++] << ((i += 11) & 0xf);
    }

    return h;
}

int
lookUpReAlloc(LUTAB *tbl, int nel) {
    int i;
    LUENT *le;
    int oldTSize;
    LUENT *oldTable;

    oldTable = tbl->tabl;
    oldTSize = tbl->tsiz;
    i = tbl->ndel;
    if ( !lookUpInit(tbl, nel) ) {
        // No more memory!
        tbl->tabl = oldTable;
        tbl->tsiz = oldTSize;
        tbl->ndel = i;
        return 0;
    }

    /*
     * The following code may fail if the user has reclaimed many
     * deleted entries and the system runs out of memory in a
     * recursive call to lu_find().
     */
    for ( i = 0, le = oldTable; i < oldTSize; i++, le++ ) {
        if ( le->key != nullptr) {
            if ( le->data != nullptr) {
                *lookUpFind(tbl, le->key) = *le;
            } else {
                if ( tbl->freek != nullptr) {
                    (*tbl->freek)(le->key);
                }
            }
        }
    }
    free(oldTable);

    return tbl->tsiz;
}

/**
Find a table entry
*/
LUENT *
lookUpFind(LUTAB *tbl, char *key)
{
    long hVal;
    int ndx;
    int i, n;
    LUENT *le;

    // Look up object
    if ( tbl->tsiz <= 0 ) {
        lookUpInit(tbl, 1);
    }

    hVal = (*tbl->hashf)(key);

    do {
        ndx = (int)(hVal % tbl->tsiz);
        le = &tbl->tabl[ndx];
        i = 0;
        n = -1;
        do {
            if ( le->key == nullptr ) {
                le->hval = hVal;
                return le;
            }
            if ( le->hval == hVal &&
                 (tbl->keycmp == nullptr || (*tbl->keycmp)(le->key, key) == 0)) {
                return le;
            }

            i++;
            n += 2;
            le += n;
            if ( (ndx += n) >= tbl->tsiz ) {
                // This happens rarely
                ndx = ndx % tbl->tsiz;
                le = &tbl->tabl[ndx];
            }
        }
        while ( i < tbl->tsiz );

        if ( !lookUpReAlloc(tbl, tbl->tsiz - tbl->ndel + 1)) {
            // Table is full, reallocate
            return nullptr;
        }
    } while ( true ); // Should happen only once!
}

/**
Free table and contents
*/
void
lookUpDone(LUTAB *l)
{
    LUENT *tp;

    if ( !l->tsiz ) {
        return;
    }
    for ( tp = l->tabl + l->tsiz; tp-- > l->tabl; ) {
        if ( tp->key != nullptr) {
            if ( l->freek != nullptr) {
                (*l->freek)(tp->key);
            }
            if ( tp->data != nullptr && l->freed != nullptr) {
                (*l->freed)(tp->data);
            }
        }
    }
    free(l->tabl);
    l->tabl = nullptr;
    l->tsiz = 0;
    l->ndel = 0;
}
