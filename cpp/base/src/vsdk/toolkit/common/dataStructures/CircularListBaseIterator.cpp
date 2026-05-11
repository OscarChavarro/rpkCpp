#include "vsdk/toolkit/common/dataStructures/CircularListBaseIterator.h"

CircularListBaseIterator::CircularListBaseIterator(CircularListBase &list): currentElement(), currentList() {
    init(list);
}

CircularListBaseIterator::~CircularListBaseIterator() {
}

void
CircularListBaseIterator::init(CircularListBase &list) {
    currentList = &list;
    currentElement = currentList->lastLink();
}

CircularListLink *
CircularListBaseIterator::next() {
    CircularListLink *response;

    if ( currentElement == nullptr ) {
        response = nullptr;
    } else {
        currentElement = currentElement->nextLink;
        response = currentElement;
    }

    if ( currentElement == currentList->lastLink() ) {
        currentElement = nullptr;
    }

    return response;
}
