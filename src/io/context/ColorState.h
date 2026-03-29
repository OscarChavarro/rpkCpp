#ifndef __COLOR_STATE__
#define __COLOR_STATE__

class ColorContext;

class ColorState {
  public:
    ColorContext *unNamedColorContext;
    ColorContext *currentColor;

    ColorState();
    ~ColorState();
};

#endif
