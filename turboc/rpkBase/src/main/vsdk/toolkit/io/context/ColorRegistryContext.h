#ifndef __COLOR_REGISTRY_CONTEXT__
#define __COLOR_REGISTRY_CONTEXT__

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

    ColorRegistryContext(const ColorRegistryContext &);
    ColorRegistryContext &operator=(const ColorRegistryContext &);
};

#endif
