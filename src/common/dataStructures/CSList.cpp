#include "common/dataStructures/CSList.h"

CISLink *CircularListBase::Remove() {
    // Remove first element and return it

    if ( last == nullptr ) {
        // Not really an error.
        // Error("CList_Base::Remove", "Remove from empty list");
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

void
CircularListBase::add(CISLink *data) {
    // Add an element to the head of the list
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

CISLink *CircularListBaseIterator::next() {
    CISLink *ret = (currentElement ?
                    (currentElement = currentElement->nextLink) :
                    nullptr);
    if ( currentElement == currentList->last ) {
        currentElement = nullptr;
    }

    return ret;
}
