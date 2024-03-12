#include "common/dataStructures/CircularList.h"

/**
Remove first element and return it
*/
CISLink *
CircularListBase::remove() {
    if ( last == nullptr ) {
        return nullptr;
    }

    CISLink *first = last->nextLink;

    if ( first == last ) {
        last = nullptr;
    } else {
        last->nextLink = first->nextLink;
    }

    return first;
}

CISLink::CISLink() {
    nextLink = nullptr;
}

CircularListBase::CircularListBase() {
    last = nullptr;
}

void
CircularListBase::clear() {
    last = nullptr;
}

/**
Add an element to the head of the list
*/
void
CircularListBase::add(CISLink *data) {
    if ( last != nullptr ) {
        // Not empty
        data->nextLink = last->nextLink;
    } else {
        last = data;
    }

    last->nextLink = data;
}

void
CircularListBase::append(CISLink *data) {
    if ( last != nullptr ) {
        data->nextLink = last->nextLink;
        last = last->nextLink = data;
    } else {
        last = data->nextLink = data;
    }
}

CircularListBaseIterator::CircularListBaseIterator(CircularListBase &list): currentElement(), currentList() {
    init(list);
}

void CircularListBaseIterator::init(CircularListBase &list) {
    currentList = &list;
    currentElement = currentList->last;
}

CISLink *
CircularListBaseIterator::next() {
    CISLink *ret = (currentElement ?
                    (currentElement = currentElement->nextLink) :
                    nullptr);
    if ( currentElement == currentList->last ) {
        currentElement = nullptr;
    }

    return ret;
}
