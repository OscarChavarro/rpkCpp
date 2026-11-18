#ifndef __OBJECT__
#define __OBJECT__

namespace java {

class Object {
  public:
    virtual ~Object();
    virtual void dispose();
};

}

#endif
