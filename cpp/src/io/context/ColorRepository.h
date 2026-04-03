#ifndef __COLOR_REPOSITORY__
#define __COLOR_REPOSITORY__

#include "io/context/ColorContext.h"
#include "io/context/LookUpTable.h"

class ColorRepository {
  public:
    LookUpTable<char *> *colorLookUpTable;
    ColorContext *unNamedColorContext;
    ColorContext *currentColor;

    ColorRepository();
    ~ColorRepository();

    void reset();

    ColorRepository(const ColorRepository &) = delete;
    ColorRepository &operator=(const ColorRepository &) = delete;
};

#endif
