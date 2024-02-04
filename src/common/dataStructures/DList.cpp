/**
List.c: generic double linked linear lists
*/

#include <cstdlib>

#include "common/dataStructures/DList.h"

/**
Prepends the element to the list, returns the modified list
 */
DLIST *DListAdd(DLIST *dlist, void *pelement) {
    if ( pelement == nullptr) {
        return dlist;
    }

    DLIST *newdlist = (DLIST *)malloc(sizeof(DLIST));
    newdlist->pelement = pelement;
    newdlist->next = dlist;
    newdlist->prev = nullptr;

    if ( dlist ) {
        dlist->prev = newdlist;
    }

    return newdlist;
}
