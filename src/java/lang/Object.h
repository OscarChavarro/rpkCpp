#ifndef __Object__
#define __Object__

namespace java {

class Object {
  public:
    virtual ~Object();
    virtual void dispose();
};

}

#endif
