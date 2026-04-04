#ifndef __OPTION_DSL__
#define __OPTION_DSL__

#include "app/options/TypedOption.h"

#define DEFINE_OPTION_INT(name, var) \
    { name, &(var), nullptr }

#define DEFINE_OPTION_INT_ABBR(name, abbr, var) \
    { name, &(var), nullptr }

#define DEFINE_OPTION_FLOAT(name, var) \
    { name, &(var), nullptr }

#define DEFINE_OPTION_FLOAT_ABBR(name, abbr, var) \
    { name, &(var), nullptr }

#define DEFINE_OPTION_BOOL(name, var) \
    { name, &(var), nullptr }

#define DEFINE_OPTION_BOOL_ABBR(name, abbr, var) \
    { name, &(var), nullptr }

#endif
