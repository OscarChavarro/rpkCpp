/**
Hierarchical object names tracking
*/

#include <cstring>

#include "io/mgf/parser.h"
#include "io/mgf/words.h"

int GLOBAL_mgf_objectNames; // depth of name hierarchy
char **GLOBAL_mgf_objectNamesList; // name list

static int globalObjectMaxName; // allocated list size

#define ALLOC_INC 16 // list increment ( > 1 )

/**
Handle an object entity statement
*/
int
handleObject2Entity(int ac, char **av)
{
    if ( ac == 1 ) {
        // just pop top object
        if ( GLOBAL_mgf_objectNames < 1 ) {
            return MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        free(GLOBAL_mgf_objectNamesList[--GLOBAL_mgf_objectNames]);
        GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = nullptr;
        return MGF_OK;
    }
    if ( ac != 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isnameWords(av[1])) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( GLOBAL_mgf_objectNames >= globalObjectMaxName - 1 ) {
        // enlarge array
        if ( !globalObjectMaxName ) {
            GLOBAL_mgf_objectNamesList = (char **) malloc(
                    (globalObjectMaxName = ALLOC_INC) * sizeof(char *));
        } else {
            GLOBAL_mgf_objectNamesList = (char **) realloc((void *) GLOBAL_mgf_objectNamesList, (globalObjectMaxName += ALLOC_INC) * sizeof(char *));
        }
        if ( GLOBAL_mgf_objectNamesList == nullptr) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    // allocate new entry
    GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = (char *) malloc(strlen(av[1]) + 1);
    if ( GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] == nullptr) {
        return MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames++], av[1]);
    GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = nullptr;
    return MGF_OK;
}
