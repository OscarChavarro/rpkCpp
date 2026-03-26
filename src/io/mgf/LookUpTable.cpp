/**
Table lookup routines
*/

#include <cstring>
#include <cstddef>

#include "io/mgf/LookUpEntity.h"
#include "io/mgf/LookUpTable.h"

namespace {
int
lookUpReAlloc(LookUpTable *tbl, int nel) {
    LookUpEntity *oldTable = tbl->getTable();
    const int oldTSize = tbl->getCurrentTableSize();
    int i = tbl->getNumberOfDeletedEntries();
    if ( !tbl->lookUpInit(nel) ) {
        // No more memory!
        tbl->setTable(oldTable);
        tbl->setCurrentTableSize(oldTSize);
        tbl->setNumberOfDeletedEntries(i);
        return 0;
    }

    // The following code may fail if the user has reclaimed many
    // deleted entries and the system runs out of memory in a
    // recursive call to lookUpFind()
    for ( i = 0; i < oldTSize; i++ ) {
        const LookUpEntity &entity = oldTable[i];
        if ( entity.key != nullptr ) {
            if ( entity.data != nullptr ) {
                *tbl->lookUpFind(entity.key) = entity;
            } else {
                if ( tbl->getFreeKeyFunction() != nullptr ) {
                    (*tbl->getFreeKeyFunction())(entity.key);
                }
            }
        }
    }
    delete[] oldTable;

    return tbl->getCurrentTableSize();
}
}

void
LookUpTable::lookUpRemove(const char *data) {
    delete[] data;
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
The LookUpTable constructor provides a convenient static declaration for character
string keys.
*/
long
LookUpTable::lookUpShuffleHash(const char *s) {
    static const unsigned char globalShuffle[256] = {
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
    int i = 0;
    long h = 0;
    const unsigned char *text = reinterpret_cast<const unsigned char *>(s);

    for ( std::size_t index = 0; text[index] != '\0'; index++ ) {
        h ^= static_cast<long>(globalShuffle[text[index]]) << ((i += 11) & 0xf);
    }

    return h;
}

LookUpTable::LookUpTable():
    LookUpTable(nullptr, nullptr)
{
}

LookUpTable::LookUpTable(
    void (*freeKeyFunction)(const char *),
    void (*freeDataFunction)(const char *)):
    keyHashFunction(lookUpShuffleHash),
    keyCompareFunction(std::strcmp),
    freeKeyFunction(freeKeyFunction),
    freeDataFunction(freeDataFunction),
    currentTableSize(0),
    table(nullptr),
    numberOfDeletedEntries(0)
{
}

long
(*LookUpTable::getKeyHashFunction() const)(const char *)
{
    return keyHashFunction;
}

int
(*LookUpTable::getKeyCompareFunction() const)(const char *, const char *)
{
    return keyCompareFunction;
}

void
(*LookUpTable::getFreeKeyFunction() const)(const char *)
{
    return freeKeyFunction;
}

void
(*LookUpTable::getFreeDataFunction() const)(const char *)
{
    return freeDataFunction;
}

int
LookUpTable::getCurrentTableSize() const
{
    return currentTableSize;
}

LookUpEntity *
LookUpTable::getTable() const
{
    return table;
}

int
LookUpTable::getNumberOfDeletedEntries() const
{
    return numberOfDeletedEntries;
}

void
LookUpTable::setCurrentTableSize(int value)
{
    currentTableSize = value;
}

void
LookUpTable::setTable(LookUpEntity *value)
{
    table = value;
}

void
LookUpTable::setNumberOfDeletedEntries(int value)
{
    numberOfDeletedEntries = value;
}

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
LookUpTable::lookUpInit(int nel) {
    static int hSizeTab[] = {
            31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
            32749, 65521, 131071, 262139, 524287, 1048573, 2097143,
            4194301, 8388593, 0
    };
    int chosenTableSize = 0;

    nel += nel >> 1; // 66% occupancy
    for ( int i = 0; hSizeTab[i] != 0; i++ ) {
        if ( hSizeTab[i] > nel ) {
            chosenTableSize = hSizeTab[i];
            break;
        }
    }
    setCurrentTableSize(chosenTableSize);
    if ( !getCurrentTableSize() ) {
        // Not always prime
        setCurrentTableSize(nel * 2 + 1);
    }
    setTable(new LookUpEntity[getCurrentTableSize()]);
    for ( int i = 0; i < getCurrentTableSize(); i++ ) {
        getTable()[i].key = nullptr;
        getTable()[i].data = nullptr;
        getTable()[i].value = 0;
    }
    if ( getTable() == nullptr ) {
        setCurrentTableSize(0);
    }
    setNumberOfDeletedEntries(0);

    return getCurrentTableSize();
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
LookUpTable::lookUpFind(const char *key) {
    // Look up object
    if ( getCurrentTableSize() <= 0 ) {
        lookUpInit(1);
    }

    const long hVal = (*getKeyHashFunction())(key);

    do {
        int ndx = static_cast<int>(hVal % getCurrentTableSize());
        int i = 0;
        int n = -1;
        do {
            LookUpEntity *le = &getTable()[ndx];
            if ( le->key == nullptr ) {
                le->value = hVal;
                return le;
            }
            if ( le->value == hVal &&
                 (getKeyCompareFunction() == nullptr || (*getKeyCompareFunction())(le->key, key) == 0) ) {
                return le;
            }

            i++;
            n += 2;
            if ( (ndx += n) >= getCurrentTableSize() ) {
                // This happens rarely
                ndx = ndx % getCurrentTableSize();
            }
        }
        while ( i < getCurrentTableSize() );

        if ( !lookUpReAlloc(this, getCurrentTableSize() - getNumberOfDeletedEntries() + 1) ) {
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
LookUpTable::lookUpDone() {
    if ( !getCurrentTableSize() ) {
        return;
    }

    for ( int i = getCurrentTableSize() - 1; i >= 0; i-- ) {
        const LookUpEntity *tp = &getTable()[i];
        if ( tp->key != nullptr ) {
            if ( getFreeKeyFunction() != nullptr ) {
                (*getFreeKeyFunction())(tp->key);
            }
            if ( tp->data != nullptr && getFreeDataFunction() != nullptr ) {
                (*getFreeDataFunction())(tp->data);
            }
        }
    }
    delete[] getTable();
    setTable(nullptr);
    setCurrentTableSize(0);
    setNumberOfDeletedEntries(0);
}
