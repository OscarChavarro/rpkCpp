#ifndef __SCOPED_ARRAY__
#define __SCOPED_ARRAY__

#include "common/VSDK.h"

template <typename T>
class ScopedArrayBuffer {
  private:
    T *value;

  public:
    explicit ScopedArrayBuffer(T *initialValue = NULL):
        value(initialValue)
    {
    }

    ~ScopedArrayBuffer() {
        delete[] value;
        value = NULL;
    }

    ScopedArrayBuffer(const ScopedArrayBuffer &);
    ScopedArrayBuffer &operator=(const ScopedArrayBuffer &);

    void
    reset(T *newValue = NULL) {
        if ( value != newValue ) {
            delete[] value;
            value = newValue;
        }
    }

    T *
    get() const {
        return value;
    }
};

#endif
