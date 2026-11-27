
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Double.h"
#include "java/lang/Float.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/NumberFormatException.h"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/EntityControlAccessor.h"
#include <limits.h>

BOOLEAN
EntityControlAccessor::isInstance(const Entity &entity, const Class &type)
{
    return entity.instanceOf(type);
}

void
EntityControlValue::parse(const String &text, double *outValue)
{
    *outValue = Double::parseDouble(text);
}

String
EntityControlValue::toString(double value)
{
    return Double::toString(value);
}

void
EntityControlValue::parse(const String &text, float *outValue)
{
    *outValue = Float::parseFloat(text);
}

String
EntityControlValue::toString(float value)
{
    return Float::toString(value);
}

void
EntityControlValue::parse(const String &text, int *outValue)
{
    *outValue = Integer::parseInt(text);
}

String
EntityControlValue::toString(int value)
{
    return String::valueOf(value);
}

#ifdef VSDK_HAS_LONG_LONG
void
EntityControlValue::parse(const String &text, long long *outValue)
{
    *outValue = Long::parseLong(text);
}

String
EntityControlValue::toString(long long value)
{
    return String::valueOf(value);
}
#endif

void
EntityControlValue::parse(const String &text, long *outValue)
{
    long long value = Long::parseLong(text);
    if ( value < LONG_MIN ||
         value > LONG_MAX ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    *outValue = ((long)(value));
}

String
EntityControlValue::toString(long value)
{
    return String::valueOf(value);
}
