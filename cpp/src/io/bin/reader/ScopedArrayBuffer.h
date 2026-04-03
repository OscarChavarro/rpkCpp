#ifndef __SCOPED_ARRAY__
#define __SCOPED_ARRAY__

template <typename T>
class ScopedArrayBuffer {
  private:
    T *value;

  public:
    explicit ScopedArrayBuffer(T *initialValue = nullptr):
        value(initialValue)
    {
    }

    ~ScopedArrayBuffer() {
        delete[] value;
        value = nullptr;
    }

    ScopedArrayBuffer(const ScopedArrayBuffer &) = delete;
    ScopedArrayBuffer &operator=(const ScopedArrayBuffer &) = delete;

    void
    reset(T *newValue = nullptr) {
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
