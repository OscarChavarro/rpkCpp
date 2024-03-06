#include <cstdlib>

#include "common/error.h"
#include "common/dataStructures/List.h"

/**
Adds an element in front of the list, returns a pointer to the new list
*/
LIST *
listAdd(LIST *list, void *element) {
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
listRemove(LIST *list, void *element) {
    LIST *p;
    LIST *q;

    if ( list == nullptr ) {
        logError("ListRemove", "attempt to remove an element from an empty list");
        return list;
    }

    if ( element == list->pelement ) {
        p = list->next;
        free(list);
        return p;
    }

    q = list;
    p = list->next;
    while ( p && p->pelement != element ) {
        q = p;
        p = p->next;
    }

    if ( !p ) {
        logError("ListRemove", "attempt to remove a non existing element from a list");
        return list;
    } else {
        q->next = p->next;
        free(p);
    }

    return list;
}

/**
Destroys a listWindow. Does not destroy the elements
*/
void
listDestroy(LIST *listWindow) {
    while ( listWindow != nullptr ) {
        LIST *listNode = listWindow->next;
        free(listWindow);
        listWindow = listNode;
    }
}
