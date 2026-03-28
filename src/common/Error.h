#ifndef __ERROR__
#define __ERROR__

class Error {
  public:
    static void error(const char *routine, const char *text, ...);
    static void warning(const char *routine, const char *text, ...);
    [[noreturn]] static void fatal(int errcode, const char *routine, const char *text, ...);
};

#endif
