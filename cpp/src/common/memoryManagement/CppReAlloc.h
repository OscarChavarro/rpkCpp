#ifndef CPP_RE_ALLOC__
#define CPP_RE_ALLOC__

class CppReAlloc {
  public:
    CppReAlloc() = delete;

    static unsigned char *reAlloc(
        unsigned char *ptr,
        int oldElementCount,
        int newElementCount);

    static char **reAlloc(
        char **ptr,
        int oldElementCount,
        int newElementCount);
};

#endif
