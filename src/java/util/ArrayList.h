#ifndef __ArrayList__
#define __ArrayList__

#include <cstdlib>
#include "java/lang/Object.h"

namespace java {
    template<class T>
    class ArrayList : public Object {
    private:
        long int increaseChunk;
        long int currentSize;
        long int maxSize;
        T *Data;

    public:
        ArrayList();

        ~ArrayList() {
            if ( Data ) {
                free(Data);
                Data = nullptr;
            }
            currentSize = 0;
            maxSize = -1;
        }

        explicit ArrayList(long i) {
            currentSize = 0;
            increaseChunk = i;
            maxSize = increaseChunk;
            init();
        };

        void
        init() {
            Data = (T *) malloc(sizeof(T) * maxSize);
            currentSize = 0;
        };

        bool add(T elem);

        long int size() const {
            return currentSize;
        }

        T &operator[](long int i) {
            return Data[i];
        }

        T get(long int i) const {
            return Data[i];
        }

        T *data() {
            return Data;
        };

        void add(long int pos, T elem);

        void remove(long int pos);

        void remove(T data);

        void set(long int pos, T elem) {
            Data[pos] = elem;
        };
    };

}

#endif
