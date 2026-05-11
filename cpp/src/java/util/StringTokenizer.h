#ifndef JAVA_STRING_TOKENIZER__
#define JAVA_STRING_TOKENIZER__

#include "java/lang/String.h"

namespace java {

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
    explicit StringTokenizer(const java::String &text, const char *delimiters = " \t\n\r\f");
    ~StringTokenizer();

    void
    dispose();

    bool
    hasMoreTokens() const;

    java::String
    nextToken();
};

}

#endif
