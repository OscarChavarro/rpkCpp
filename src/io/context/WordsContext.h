#ifndef __WORDS_CONTEXT__
#define __WORDS_CONTEXT__

class WordsContext {
  public:
    static int isIntDelimited(const char *text, const char *delimiters);
    static int isFloatDelimited(const char *text, const char *delimiters);
    static int isFloat(const char *text);
    static int isInt(const char *text);
    static int isName(const char *text);

  private:
    static bool isAsciiCode(int value);
    static bool isAsciiGraph(int value);
    static const char *skipInt(const char *text);
    static const char *skipFloat(const char *text);
};

#endif
