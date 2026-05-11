#ifndef CIRCULAR_LIST_BASE__
#define CIRCULAR_LIST_BASE__

#include "vsdk/toolkit/common/dataStructures/CircularListLink.h"

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
    CircularListLink *lastLink() const;
};

#endif
