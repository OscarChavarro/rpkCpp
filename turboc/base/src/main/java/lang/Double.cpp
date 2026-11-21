#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Double.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/NumberFormatException.h"
#include "vsdk/toolkit/common/Cpp98Support.h"


static BOOLEAN isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

/**
Parses an optional sign followed by decimal digits (already validated), as
`strtoll` (missing in old C libraries, i.e. libc 5), accumulating a negative
value as Java's `Long.parseLong` does, so `Long.MIN_VALUE` is covered.
@return false on overflow of `long long`
*/
static BOOLEAN parseDecimalLongLong(const char* text, long long* outValue)
{
    BOOLEAN negative = FALSE;
    if ( *text == '+' || *text == '-' ) {
        negative = *text == '-';
        text++;
    }
    long long limit = negative ? Long::MIN_VALUE : -Long::MAX_VALUE;
    long long multiplyLimit = limit / 10;
    long long value = 0;
    for ( ; *text != '\0'; text++ ) {
        int digit = *text - '0';
        if ( value < multiplyLimit ) {
            return FALSE;
        }
        value *= 10;
        if ( value < limit + digit ) {
            return FALSE;
        }
        value -= digit;
    }
    *outValue = negative ? value : -value;
    return TRUE;
}

double Double::parseDouble(const String& text)
{
    const char* start = text.c_str();
    while ( isSpace(*start) ) {
        start++;
    }
    if ( *start == '\0' ) {
        String message = text;
        if ( text.isEmpty() ) {
            message = "empty String";
        }
        throw NumberFormatException(message);
    }
    char* end = NULL;
    double value = strtod(start, &end);
    if ( end == start ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    // Java accepts an optional float/double suffix
    if ( *end == 'd' || *end == 'D' || *end == 'f' || *end == 'F' ) {
        end++;
    }
    while ( isSpace(*end) ) {
        end++;
    }
    if ( *end != '\0' ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    return value;
}

String Double::toString(double value)
{
    if ( Cpp98Support::isNaN(value) ) {
        return "NaN";
    }
    if ( Cpp98Support::isInfinite(value) ) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    if ( value == 0.0 ) {
        return Cpp98Support::signBit(value) ? "-0.0" : "0.0";
    }

    // Shortest mantissa that reads back as the same number
    char buffer[64];
    int precision;
    for ( precision = 1; precision <= 17; precision++ ) {
        snprintf(buffer, sizeof(buffer), "%.*e", precision - 1, value);
        if ( strtod(buffer, NULL) == value ) {
            break;
        }
    }

    // buffer is "[-]d.ddde[+-]xx": split digits and exponent
    BOOLEAN negative = buffer[0] == '-';
    const char* p = negative ? buffer + 1 : buffer;
    char digits[32];
    int digitCount = 0;
    while ( *p != 'e' && *p != '\0' ) {
        if ( *p != '.' ) {
            digits[digitCount++] = *p;
        }
        p++;
    }
    digits[digitCount] = '\0';
    int exponent = (*p == 'e') ? atoi(p + 1) : 0;
    while ( digitCount > 1 && digits[digitCount - 1] == '0' ) {
        digits[--digitCount] = '\0';
    }

    String out = negative ? "-" : "";
    char one[2] = {'\0', '\0'};
    double magnitude = fabs(value);

    if ( magnitude >= 1e-3 && magnitude < 1e7 ) {
        if ( exponent >= 0 ) {
            int i;
            for ( i = 0; i <= exponent; i++ ) {
                one[0] = i < digitCount ? digits[i] : '0';
                out += one;
            }
            out += ".";
            if ( exponent + 1 >= digitCount ) {
                out += "0";
            }
            else {
                out += digits + exponent + 1;
            }
        }
        else {
            out += "0.";
            int i;
            for ( i = 0; i < -exponent - 1; i++ ) {
                out += "0";
            }
            out += digits;
        }
        return out;
    }

    one[0] = digits[0];
    out += one;
    out += ".";
    out += digitCount > 1 ? ((const char *)(digits + 1)) : "0";
    out += "E";
    out += String::valueOf(exponent);
    return out;
}

int Integer::parseInt(const String& text)
{
    const char* start = text.c_str();
    if ( *start == '\0' ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    const char* p = start;
    if ( *p == '+' || *p == '-' ) {
        p++;
    }
    if ( *p == '\0' ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    for ( const char* q = p; *q != '\0'; q++ ) {
        if ( *q < '0' || *q > '9' ) {
            throw NumberFormatException(
                String("For input string: \"") + text + "\"");
        }
    }
    long long value = 0;
    if ( !parseDecimalLongLong(start, &value) ||
         value < MIN_VALUE || value > MAX_VALUE ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    return (int)value;
}

long long Long::parseLong(const String& text)
{
    const char* start = text.c_str();
    const char* p = start;
    if ( *p == '+' || *p == '-' ) {
        p++;
    }
    if ( *p == '\0' ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    for ( const char* q = p; *q != '\0'; q++ ) {
        if ( *q < '0' || *q > '9' ) {
            throw NumberFormatException(
                String("For input string: \"") + text + "\"");
        }
    }
    long long value = 0;
    if ( !parseDecimalLongLong(start, &value) ) {
        throw NumberFormatException(
            String("For input string: \"") + text + "\"");
    }
    return value;
}


int Double::compare(double d1, double d2)
{
    if ( d1 < d2 ) {
        return -1;
    }
    if ( d1 > d2 ) {
        return 1;
    }

    // Values are equal or some of them is NaN: compare as Java
    // `doubleToLongBits`, where NaN is canonical and sorts last, and -0.0
    // sorts before 0.0
    BOOLEAN nan1 = Cpp98Support::isNaN(d1);
    BOOLEAN nan2 = Cpp98Support::isNaN(d2);
    if ( nan1 || nan2 ) {
        return nan1 == nan2 ? 0 : (nan1 ? 1 : -1);
    }
    BOOLEAN negative1 = Cpp98Support::signBit(d1);
    BOOLEAN negative2 = Cpp98Support::signBit(d2);
    if ( negative1 == negative2 ) {
        return 0;
    }
    return negative1 ? -1 : 1;
}

const double Double::MIN_VALUE = 4.9406564584124654e-324;
const double Double::MAX_VALUE = 1.7976931348623157e+308;
const double Double::MIN_NORMAL = 2.2250738585072014e-308;
const double Double::POSITIVE_INFINITY = HUGE_VAL;
const double Double::NEGATIVE_INFINITY = -HUGE_VAL;
const double Double::NaN = Cpp98Support::nan();


BOOLEAN Double::isNaN(double value)
{
    return value != value;
}

BOOLEAN Double::isFinite(double value)
{
    return value >= -MAX_VALUE && value <= MAX_VALUE;
}
