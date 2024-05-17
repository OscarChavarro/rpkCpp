#ifndef __CIRCULAR_LIST_BASE__
#define __CIRCULAR_LIST_BASE__

#include "common/dataStructures/CircularListLink.h"

class CircularListBase {
  private:
    CircularListLink *last;

  public:
    CircularListBase();
    virtual ~CircularListBase();
    virtual void addLink(CircularListLink *data);
    virtual void appendLink(CircularListLink *data);
    virtual CircularListLink *remove();
    virtual void clear();

    friend class CircularListBaseIterator;
};

#endif
