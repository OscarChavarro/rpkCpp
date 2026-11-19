#ifndef __MEMORY_POOL__
#define __MEMORY_POOL__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Object.h"

/**
Storage block of a `MemoryPool` (not nested in it, so it can be named
without `typename`, unknown to legacy compilers).
*/
struct MemoryPoolBlock {
    char *buffer;
    long int capacityElements;
    long int usedElements;
    MemoryPoolBlock *prev;
    MemoryPoolBlock *next;
};

template <class T>
class MemoryPool : public Object {
  private:
    typedef MemoryPoolBlock Block;

    Block *head;
    Block *tail;
    Block *current;
    BOOLEAN initialized;

    static Block *createBlock(long int capacityElements);
    static void destroyBlock(Block *block);

  public:
    MemoryPool();
    ~MemoryPool();

    void init(long int sizeInBytes);
    T *allocate(int numberOfElements);
    void free(int numberOfElements);
    void clear();
    BOOLEAN expand(int numberOfElements);
};


#endif
