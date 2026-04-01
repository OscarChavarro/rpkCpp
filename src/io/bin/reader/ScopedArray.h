#ifndef __SCOPED_ARRAY__
#define __SCOPED_ARRAY__

template <typename T>
class ScopedArray {
  private:
    T *value;

  public:
    explicit ScopedArray(T *initialValue = nullptr):
        value(initialValue)
    {
    }

    ~ScopedArray() {
        delete[] value;
        value = nullptr;
    }

    ScopedArray(const ScopedArray &) = delete;
    ScopedArray &operator=(const ScopedArray &) = delete;

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
