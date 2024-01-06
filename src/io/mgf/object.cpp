/**
Hierarchical object names tracking
*/

#include <cstring>

#include "io/mgf/parser.h"
#include "io/mgf/words.h"

int obj_nnames; // depth of name hierarchy
char **obj_name; // name list

static int obj_maxname; // allocated list size

#define ALLOC_INC 16 // list increment ( > 1 )

/**
Handle an object entity statement
*/
int
obj_handler(int ac, char **av)
{
    if ( ac == 1 ) {
        // just pop top object
        if ( obj_nnames < 1 ) {
            return MG_ECNTXT;
        }
        free((MEM_PTR) obj_name[--obj_nnames]);
        obj_name[obj_nnames] = nullptr;
        return MG_OK;
    }
    if ( ac != 2 ) {
        return MG_EARGC;
    }
    if ( !isnameWords(av[1])) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( obj_nnames >= obj_maxname - 1 ) {
        // enlarge array
        if ( !obj_maxname ) {
            obj_name = (char **) malloc(
                    (obj_maxname = ALLOC_INC) * sizeof(char *));
        } else {
            obj_name = (char **) realloc((MEM_PTR) obj_name, (obj_maxname += ALLOC_INC) * sizeof(char *));
        }
        if ( obj_name == nullptr) {
            return MG_EMEM;
        }
    }

    // allocate new entry
    obj_name[obj_nnames] = (char *) malloc(strlen(av[1]) + 1);
    if ( obj_name[obj_nnames] == nullptr) {
        return MG_EMEM;
    }
    strcpy(obj_name[obj_nnames++], av[1]);
    obj_name[obj_nnames] = nullptr;
    return MG_OK;
}
