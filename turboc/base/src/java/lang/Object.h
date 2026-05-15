#ifndef __Object__
#define __Object__

#include "common/VSDK.h"


class Object {
  public:
    virtual ~Object();
    virtual void dispose();
};


#endif
