#ifndef COLOR_REPOSITORY__
#define COLOR_REPOSITORY__

#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/common/dataStructures/LookUpTable.h"

class ColorRegistryContext {
  public:
    LookUpTable<char *> *colorLookUpTable;
    ColorContext *unNamedColorContext;
    ColorContext *currentColor;

    ColorRegistryContext();
    ~ColorRegistryContext();

    void reset();

    ColorRegistryContext(const ColorRegistryContext &) = delete;
    ColorRegistryContext &operator=(const ColorRegistryContext &) = delete;
};

#endif
