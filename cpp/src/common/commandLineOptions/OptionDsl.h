#ifndef __OPTION_DSL__
#define __OPTION_DSL__

#include "common/commandLineOptions/TypedOption.h"

#define DEFINE_OPTION_INT(name, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_OPTION_INT_ABBR(name, abbr, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_OPTION_FLOAT(name, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_OPTION_FLOAT_ABBR(name, abbr, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_OPTION_BOOL(name, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_OPTION_BOOL_ABBR(name, abbr, var) \
    { name, &(var), 1, nullptr, nullptr }

#define DEFINE_FLAG_BOOL(name, var) \
    { name, &(var), 0, nullptr, nullptr }

#endif
