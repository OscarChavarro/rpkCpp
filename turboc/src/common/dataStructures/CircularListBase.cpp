#include "common/dataStructures/CircularListBase.h"

CircularListBase::CircularListBase() {
    last = NULL;
}

CircularListBase::~CircularListBase() {
}
/**
Remove first element and return it
*/
CircularListLink *
CircularListBase::remove() {
    if ( last == NULL ) {
        return NULL;
    }

    CircularListLink *first = last->nextLink;

    if ( first == last ) {
        last = NULL;
    } else {
        last->nextLink = first->nextLink;
    }

    return first;
}

void
CircularListBase::clear() {
    last = NULL;
}

CircularListLink *
CircularListBase::lastLink() const {
    return last;
}

/**
Add an element to the head of the list
*/
void
CircularListBase::addLink(CircularListLink *data) {
    if ( last != NULL ) {
        // Not empty
        data->nextLink = last->nextLink;
    } else {
        last = data;
    }

    last->nextLink = data;
}

void
CircularListBase::appendLink(CircularListLink *data) {
    if ( last != NULL ) {
        data->nextLink = last->nextLink;
        last = last->nextLink = data;
    } else {
        last = data->nextLink = data;
    }
}
