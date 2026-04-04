#ifndef __OPTION_DSL__
#define __OPTION_DSL__

#include "app/options/TypedOption.h"
#include "app/options/ValueParser.h"

#define DEFINE_OPTION_INT(name, var) \
    { name, 0, &(var), ValueParser<int>::parse, nullptr }

#define DEFINE_OPTION_INT_ABBR(name, abbr, var) \
    { name, abbr, &(var), ValueParser<int>::parse, nullptr }

#define DEFINE_OPTION_FLOAT(name, var) \
    { name, 0, &(var), ValueParser<float>::parse, nullptr }

#define DEFINE_OPTION_FLOAT_ABBR(name, abbr, var) \
    { name, abbr, &(var), ValueParser<float>::parse, nullptr }

#define DEFINE_OPTION_BOOL(name, var) \
    { name, 0, &(var), ValueParser<bool>::parse, nullptr }

#define DEFINE_OPTION_BOOL_ABBR(name, abbr, var) \
    { name, abbr, &(var), ValueParser<bool>::parse, nullptr }

#endif
