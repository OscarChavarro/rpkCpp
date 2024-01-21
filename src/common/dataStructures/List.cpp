#include <cstdlib>

#include "common/dataStructures/List.h"
#include "common/error.h"

void *GLOBAL_listHandler;

/**
Adds an element in front of the list, returns a pointer to the new list
*/
LIST *
ListAdd(LIST *list, void *element) {
    if ( element == (void *) nullptr) {
        return list;
    }

    LIST *newListNode = (LIST *)malloc(sizeof(LIST));
    newListNode->pelement = element;
    newListNode->next = list;

    return newListNode;
}

/**
Counts the number of elements in a list
*/
int
ListCount(LIST *list) {
    int count = 0;

    while ( list ) {
        count++;
        list = list->next;
    }

    return count;
}

/**
merge two lists: the elements of list2 are prepended to the elements
of list1. Returns a pointer to the enlarged list1
*/
LIST *
ListMerge(LIST *list1, LIST *list2) {
    void *pelement = (void *) nullptr;

    while ((pelement = ListNext(&list2))) {
        list1 = ListAdd(list1, pelement);
    }

    return list1;
}

/**
Duplicates a list: the elements are not duplicated: only a pointer
to the elements is copied
*/
LIST *
ListDuplicate(LIST *list) {
    LIST *newlist = (LIST *) nullptr;
    void *pelement;

    while ((pelement = ListNext(&list))) {
        newlist = ListAdd(newlist, pelement);
    }

    return newlist;
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
