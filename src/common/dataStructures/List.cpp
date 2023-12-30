#include <cstdlib>

#include "common/dataStructures/List.h"
#include "common/error.h"

void *__plistel__;

LIST *
ListAdd(LIST *list, void *pelement) {
    if ( pelement == (void *) nullptr) {
        return list;
    }

    LIST *newlist = (LIST *)malloc(sizeof(LIST));
    newlist->pelement = pelement;
    newlist->next = list;

    return newlist;
}

int
ListCount(LIST *list) {
    int count = 0;

    while ( list ) {
        count++;
        list = list->next;
    }

    return count;
}

LIST *
ListMerge(LIST *list1, LIST *list2) {
    void *pelement = (void *) nullptr;

    while ((pelement = ListNext(&list2))) {
        list1 = ListAdd(list1, pelement);
    }

    return list1;
}

LIST *
ListDuplicate(LIST *list) {
    LIST *newlist = (LIST *) nullptr;
    void *pelement;

    while ((pelement = ListNext(&list))) {
        newlist = ListAdd(newlist, pelement);
    }

    return newlist;
}

LIST *
ListRemove(LIST *list, void *pelement) {
    LIST *p, *q;

    if ( !list ) {
        Error("ListRemove", "attempt to remove an element from an empty list");
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
        Error("ListRemove", "attempt to remove a nonexisting element from a list");
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

void
ListDestroy(LIST *list) {
    LIST *p;

    while ( list ) {
        p = list->next;
        free(list);
        list = p;
    }
}
