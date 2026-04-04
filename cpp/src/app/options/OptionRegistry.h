#ifndef __OPTION_REGISTRY__
#define __OPTION_REGISTRY__

#include "app/options/TypedOption.h"

template<typename T>
struct OptionRegistry {
    Option<T> *options;
    int count;
};

#endif
