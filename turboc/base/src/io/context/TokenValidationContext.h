#ifndef __WORDS_CONTEXT__
#define __WORDS_CONTEXT__

#include "common/VSDK.h"

class TokenValidationContext {
  public:
    static bool isIntDelimited(const char *text, const char *delimiters);
    static bool isFloatDelimited(const char *text, const char *delimiters);
    static bool isFloat(const char *text);
    static bool isInt(const char *text);
    static bool isName(const char *text);

  private:
    static bool isAsciiCode(int value);
    static bool isAsciiGraph(int value);
    static const char *skipInt(const char *text);
    static const char *skipFloat(const char *text);
};

#endif
