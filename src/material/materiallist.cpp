#include <cstring>

#include "material/materiallist.h"

/* Looks up a material with given name in the given material list. Returns
 * a pointer to the material if found, or (MATERIAL *)nullptr if not found. */
MATERIAL *MaterialLookup(MATERIALLIST *MaterialLib, char *name) {
    MATERIAL *m;

    while ((m = MaterialListNext(&MaterialLib))) {
        if ( strcmp(m->name, name) == 0 ) {
            return m;
        }
    }

    return (MATERIAL *)nullptr;
}
