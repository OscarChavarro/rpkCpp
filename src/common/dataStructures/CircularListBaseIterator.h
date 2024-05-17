#ifndef RPK_CIRCULARLISTBASEITERATOR_H
#define RPK_CIRCULARLISTBASEITERATOR_H

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
