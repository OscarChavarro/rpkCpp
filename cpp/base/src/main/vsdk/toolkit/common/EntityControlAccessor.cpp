#include "java/lang/String.h"

#include "java/lang/Double.h"
#include "java/lang/Float.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/NumberFormatException.h"
#include "vsdk/toolkit/common/EntityControlAccessor.h"

double
EntityControlValue<double>::parse(const java::String &text)
{
    return java::Double::parseDouble(text);
}

java::String
EntityControlValue<double>::toString(double value)
{
    return java::Double::toString(value);
}

float
EntityControlValue<float>::parse(const java::String &text)
{
    return java::Float::parseFloat(text);
}

java::String
EntityControlValue<float>::toString(float value)
{
    return java::Float::toString(value);
}

int
EntityControlValue<int>::parse(const java::String &text)
{
    return java::Integer::parseInt(text);
}

java::String
EntityControlValue<int>::toString(int value)
{
    return java::String::valueOf(value);
}

long long
EntityControlValue<long long>::parse(const java::String &text)
{
    return java::Long::parseLong(text);
}

java::String
EntityControlValue<long long>::toString(long long value)
{
    return java::String::valueOf(value);
}

long
EntityControlValue<long>::parse(const java::String &text)
{
    long long value = java::Long::parseLong(text);
    if ( value < java::Long::MIN_VALUE ||
         value > java::Long::MAX_VALUE ) {
        throw java::NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    return static_cast<long>(value);
}

java::String
EntityControlValue<long>::toString(long value)
{
    return java::String::valueOf(value);
}
