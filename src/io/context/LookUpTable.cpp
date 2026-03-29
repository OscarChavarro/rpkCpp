/**
Table lookup routines
*/

#include <cstring>

#include "io/context/LookUpEntity.h"
#include "io/context/LookUpTable.h"

namespace {
/**
Hash a null-terminated string
*/
long
lookUpShuffleHash(const char *text) {
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

    int bitShift = 0;
    long hash = 0;
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(text);

    for ( int i = 0; bytes[i] != '\0'; i++ ) {
        hash ^= static_cast<long>(globalShuffle[bytes[i]]) << ((bitShift += 11) & 0xf);
    }

    return hash;
}
}

long
StringLookUpBehavior::hash(const char *key) const {
    return lookUpShuffleHash(key);
}

bool
StringLookUpBehavior::keysEqual(const char *left, const char *right) const {
    return std::strcmp(left, right) == 0;
}

void
OwningCStringLookUpBehavior::freeKey(const char *key) const {
    delete[] key;
}

void
OwningCStringLookUpBehavior::freeData(const char *data) const {
    delete[] data;
}

namespace LookUpBehaviors {
const LookUpBehavior &
nonOwningCString() {
    // Process-lifetime singleton avoids static destruction order issues.
    static const LookUpBehavior *behavior = new StringLookUpBehavior();
    return *behavior;
}

const LookUpBehavior &
owningCString() {
    // Process-lifetime singleton avoids static destruction order issues.
    static const LookUpBehavior *behavior = new OwningCStringLookUpBehavior();
    return *behavior;
}
}

LookUpTable::LookUpTable():
    LookUpTable(LookUpBehaviors::nonOwningCString())
{
}

LookUpTable::LookUpTable(const LookUpBehavior &behavior):
    behavior(behavior),
    currentTableSize(0),
    table(nullptr),
    numberOfDeletedEntries(0)
{
}

LookUpTable::~LookUpTable() {
    lookUpDone();
}

int
LookUpTable::getCurrentTableSize() const
{
    return currentTableSize;
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

    currentTableSize = chosenTableSize;
    if ( currentTableSize == 0 ) {
        // Not always prime
        currentTableSize = nel * 2 + 1;
    }

    table = new LookUpEntity[currentTableSize];
    if ( table == nullptr ) {
        currentTableSize = 0;
        return 0;
    }

    for ( int i = 0; i < currentTableSize; i++ ) {
        table[i].key = nullptr;
        table[i].data = nullptr;
        table[i].value = 0;
    }
    numberOfDeletedEntries = 0;

    return currentTableSize;
}

/**
Find a table entry

The lookUpFind routine returns the entry corresponding to the given
key.  If the entry does not exist, the corresponding key field will
be nullptr.  If the entry has been previously deleted but not yet freed,
then only the data field will be nullptr.  It is the caller's
responsibility to (allocate and) assign the key and data fields when
creating a new entry.  The only case where lookUpFind returns nullptr is when
the system has run out of memory.
*/
LookUpEntity *
LookUpTable::lookUpFind(const char *key) {
    // Look up object
    if ( currentTableSize <= 0 ) {
        if ( !lookUpInit(1) ) {
            return nullptr;
        }
    }

    const long hashValue = behavior.hash(key);

    do {
        int index = static_cast<int>(hashValue % currentTableSize);
        int i = 0;
        int n = -1;
        do {
            LookUpEntity *entry = &table[index];
            if ( entry->key == nullptr ) {
                entry->value = hashValue;
                return entry;
            }
            if ( entry->value == hashValue && behavior.keysEqual(entry->key, key) ) {
                return entry;
            }

            i++;
            n += 2;
            if ( (index += n) >= currentTableSize ) {
                // This happens rarely
                index = index % currentTableSize;
            }
        } while ( i < currentTableSize );

        // Table is full, reallocate
        if ( !lookUpReAlloc(currentTableSize - numberOfDeletedEntries + 1) ) {
            return nullptr;
        }
    } while ( true ); // Should happen only once!
}

/**
Free table and contents

The lookUpDone routine calls the configured cleanup behavior once for each
assigned table entry (i.e. each entry with an assigned key value).
The final action of lookUpDone is to free the allocated table itself.
*/
void
LookUpTable::lookUpDone() {
    if ( currentTableSize <= 0 ) {
        return;
    }

    for ( int i = currentTableSize - 1; i >= 0; i-- ) {
        const LookUpEntity *entry = &table[i];
        if ( entry->key != nullptr ) {
            behavior.freeKey(entry->key);
            if ( entry->data != nullptr ) {
                behavior.freeData(entry->data);
            }
        }
    }

    delete[] table;
    table = nullptr;
    currentTableSize = 0;
    numberOfDeletedEntries = 0;
}

/**
Reallocate table for at least nel entries
*/
int
LookUpTable::lookUpReAlloc(int nel) {
    LookUpEntity *oldTable = table;
    const int oldTableSize = currentTableSize;
    const int oldDeletedEntries = numberOfDeletedEntries;

    if ( !lookUpInit(nel) ) {
        // No more memory!
        table = oldTable;
        currentTableSize = oldTableSize;
        numberOfDeletedEntries = oldDeletedEntries;
        return 0;
    }

    // The following code may fail if the user has reclaimed many
    // deleted entries and the system runs out of memory in a
    // recursive call to lookUpFind()
    for ( int i = 0; i < oldTableSize; i++ ) {
        const LookUpEntity &entry = oldTable[i];
        if ( entry.key == nullptr ) {
            continue;
        }
        if ( entry.data != nullptr ) {
            LookUpEntity *newEntry = lookUpFind(entry.key);
            if ( newEntry == nullptr ) {
                continue;
            }
            *newEntry = entry;
        } else {
            behavior.freeKey(entry.key);
        }
    }
    delete[] oldTable;

    return currentTableSize;
}
