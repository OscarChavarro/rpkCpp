#include <cstdlib>

#include "common/error.h"
#include "common/dataStructures/List.h"

/**
Adds an element in front of the list, returns a pointer to the new list
*/
LIST *
ListAdd(LIST *list, void *element) {
    if ( element == nullptr ) {
        return list;
    }

    LIST *newListNode = (LIST *)malloc(sizeof(LIST));
    newListNode->pelement = element;
    newListNode->next = list;

    return newListNode;
}

/**
Removes an element from the list. Returns a pointer to the updated list
*/
LIST *
ListRemove(LIST *list, void *pelement) {
    LIST *p, *q;

    if ( !list ) {
        logError("ListRemove", "attempt to remove an element from an empty list");
        return list;
    }

    if ( pelement == list->pelement ) {
        p = list->next;
        free(list);
        return p;
    }

    /* chasing pointers: */
    q = list;
    p = list->next;
    while ( p && p->pelement != pelement ) {
        q = p;
        p = p->next;
    }

    /* als p de nullptr pointer is komt het te verwijderen element niet in de lijst
     * voor: fout */
    if ( !p ) {
        logError("ListRemove", "attempt to remove a nonexisting element from a list");
        return list;
    }

        /* in het andere geval is p een wijzer naar de lijst-cel die eruit gehaald moet
         * worden */
    else {
        q->next = p->next;
        free(p);
    }

    /* geef een (ongewijzigde) wijzer naar de gewijzigde lijst terug */
    return list;
}

/**
Destroys a listWindow. Does not destroy the elements
*/
void
ListDestroy(LIST *listWindow) {
    while ( listWindow != nullptr ) {
        LIST *listNode = listWindow->next;
        free(listWindow);
        listWindow = listNode;
    }
}
