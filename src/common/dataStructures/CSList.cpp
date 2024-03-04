#include "common/dataStructures/CSList.h"

CISLink *CircularListBase::Remove() {
    // Remove first element and return it

    if ( last == nullptr ) {
        // Not really an error.
        // Error("CList_Base::Remove", "Remove from empty list");
        return nullptr;
    }

    CISLink *first = last->next;

    if ( first == last ) {
        last = nullptr;
    } else {
        last->next = first->next;
    }

    return first;
}

CISLink::CISLink() {
    next = nullptr;
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
        data->next = last->next;
    } else {
        last = data;
    }

    last->next = data;
}

void
CircularListBase::append(CISLink *data) {
    if ( last != nullptr ) {
        data->next = last->next;
        last = last->next = data;
    } else {
        last = data->next = data;
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
                    (currentElement = currentElement->next) :
                    nullptr);
    if ( currentElement == currentList->last ) {
        currentElement = nullptr;
    }

    return ret;
}
