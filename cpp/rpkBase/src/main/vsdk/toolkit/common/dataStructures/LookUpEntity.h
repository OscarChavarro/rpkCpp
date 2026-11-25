#ifndef __LOOK_UP_ENTITY__
#define __LOOK_UP_ENTITY__

template<typename T>
class LookUpEntity {
  public:
    LookUpEntity():
        key(nullptr),
        value(0),
        data()
    {
    }

    char *key; // Key name
    long value; // Key hash value (for efficiency)
    T data; // Client data
};

#endif
