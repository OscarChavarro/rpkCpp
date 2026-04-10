#ifndef __JAVA_STRING_TOKENIZER__
#define __JAVA_STRING_TOKENIZER__

#include "java/lang/String.h"


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
    explicit StringTokenizer(const String &text, const char *delimiters = " \t\n\r\f");
    ~StringTokenizer();

    void
    dispose();

    bool
    hasMoreTokens() const;

    String
    nextToken();
};


#endif
