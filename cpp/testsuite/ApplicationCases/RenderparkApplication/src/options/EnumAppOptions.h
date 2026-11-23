#ifndef APP_OPTIONS_CONTEXT__
#define APP_OPTIONS_CONTEXT__

class EnumAppOptions {
  public:
    EnumAppOptions(int inWidth, int inHeight, int inNqcdivs):
        width(inWidth), height(inHeight), nqcdivs(inNqcdivs), yesValue(1), noValue(0), debug(0) {}

    int width;
    int height;
    int nqcdivs;
    int yesValue;
    int noValue;
    int debug;
};

#endif
