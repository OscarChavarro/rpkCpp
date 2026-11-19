#ifndef __STRING_TOKENIZER__
#define __STRING_TOKENIZER__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

class StringTokenizer {
  private:
    char *text;
    char *delimiters;
    int cursor;

    BOOLEAN
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

    BOOLEAN
    hasMoreTokens() const;

    String
    nextToken();
};


#endif
