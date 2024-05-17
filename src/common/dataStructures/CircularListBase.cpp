#include "common/dataStructures/CircularListBase.h"

CircularListBase::CircularListBase() {
    last = nullptr;
}

CircularListBase::~CircularListBase() {
}
/**
Remove first element and return it
*/
CircularListLink *
CircularListBase::remove() {
    if ( last == nullptr ) {
        return nullptr;
    }

    CircularListLink *first = last->nextLink;

    if ( first == last ) {
        last = nullptr;
    } else {
        last->nextLink = first->nextLink;
    }

    return first;
}

void
CircularListBase::clear() {
    last = nullptr;
}

/**
Add an element to the head of the list
*/
void
CircularListBase::addLink(CircularListLink *data) {
    if ( last != nullptr ) {
        // Not empty
        data->nextLink = last->nextLink;
    } else {
        last = data;
    }

    last->nextLink = data;
}

void
CircularListBase::appendLink(CircularListLink *data) {
    if ( last != nullptr ) {
        data->nextLink = last->nextLink;
        last = last->nextLink = data;
    } else {
        last = data->nextLink = data;
    }
}
