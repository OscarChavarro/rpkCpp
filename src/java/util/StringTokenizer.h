#ifndef __JAVA_STRING_TOKENIZER__
#define __JAVA_STRING_TOKENIZER__

#include "java/lang/String.h"

namespace java {
namespace util {

class StringTokenizer {
  private:
    char *text;
    char *delimiters;
    int cursor;

    bool
    isDelimiter(char ch) const;

    int
    findTokenStart(int from) const;

    int
    findTokenEnd(int from) const;

  public:
    StringTokenizer(const java::lang::String &text, const char *delimiters = " \t\n\r\f");
    ~StringTokenizer();

    void
    dispose();

    bool
    hasMoreTokens() const;

    java::lang::String
    nextToken();
};

}
}

#endif
