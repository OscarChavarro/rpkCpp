/**
List.c: generic double linked linear lists
*/

#include <cstdlib>

#include "common/dataStructures/DList.h"

/**
Prepends the element to the list, returns the modified list
 */
DLIST *
listAdd(DLIST *list, void *element) {
    if ( element == nullptr) {
        return list;
    }

    DLIST *newNode = (DLIST *)malloc(sizeof(DLIST));
    newNode->element = element;
    newNode->next = list;

    return newNode;
}
