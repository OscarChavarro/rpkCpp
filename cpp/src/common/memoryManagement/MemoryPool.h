#ifndef MEMORY_POOL__
#define MEMORY_POOL__

#include "java/lang/Object.h"

template <class T>
class MemoryPool final : public java::Object {
  private:
    struct Block {
        char *buffer;
        long int capacityElements;
        long int usedElements;
        Block *prev;
        Block *next;
    };

    Block *head;
    Block *tail;
    Block *current;
    bool initialized;

    static Block *createBlock(long int capacityElements);
    static void destroyBlock(Block *block);

  public:
    MemoryPool();
    ~MemoryPool() final;

    void init(long int sizeInBytes);
    T *allocate(int numberOfElements);
    void free(int numberOfElements);
    void clear();
    bool expand(int numberOfElements);
};


#endif
