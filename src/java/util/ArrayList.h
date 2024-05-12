#ifndef __ArrayList__
#define __ArrayList__

#include <cstdlib>
#include "java/lang/Object.h"

namespace java {
    template<class T>
    class ArrayList final : public Object {
    private:
        long int increaseChunk;
        long int currentSize;
        long int maxSize;
        T *Data;

        void init();

    public:
        ArrayList();
        explicit ArrayList(long i);
        ~ArrayList();

        long int size() const;
        T get(long int i) const;

        bool add(T elem);
        T &operator[](long int i);
        T *data();
        void add(long int pos, T elem);
        void remove(long int pos);
        void remove(T data);
        void set(long int pos, T elem);
    };
}

#endif
