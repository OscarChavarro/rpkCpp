#ifndef __CIRCULAR_LIST_BASE_ITERATOR__
#define __CIRCULAR_LIST_BASE_ITERATOR__

#include "common/dataStructures/CircularListBase.h"

class CircularListBaseIterator {
  private:
    CircularListLink *currentElement;
    CircularListBase *currentList;

  public:
    explicit CircularListBaseIterator(CircularListBase &list);
    virtual ~CircularListBaseIterator();
    void init(CircularListBase &list);
    virtual CircularListLink *next();
};

#endif
