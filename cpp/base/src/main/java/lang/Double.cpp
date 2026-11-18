#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "java/lang/Double.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/NumberFormatException.h"

namespace java {

namespace {
bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}
}

double Double::parseDouble(const java::String& text)
{
    const char* start = text.c_str();
    while ( isSpace(*start) ) {
        start++;
    }
    if ( *start == '\0' ) {
        throw NumberFormatException(
            text.isEmpty() ? java::String("empty String") : text);
    }
    char* end = nullptr;
    double value = std::strtod(start, &end);
    if ( end == start ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
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
            java::String("For input string: \"") + text + "\"");
    }
    return value;
}

java::String Double::toString(double value)
{
    if ( std::isnan(value) ) {
        return "NaN";
    }
    if ( std::isinf(value) ) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    if ( value == 0.0 ) {
        return std::signbit(value) ? "-0.0" : "0.0";
    }

    // Shortest mantissa that reads back as the same number
    char buffer[64];
    int precision;
    for ( precision = 1; precision <= 17; precision++ ) {
        std::snprintf(buffer, sizeof(buffer), "%.*e", precision - 1, value);
        if ( std::strtod(buffer, nullptr) == value ) {
            break;
        }
    }

    // buffer is "[-]d.ddde[+-]xx": split digits and exponent
    bool negative = buffer[0] == '-';
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
    int exponent = (*p == 'e') ? std::atoi(p + 1) : 0;
    while ( digitCount > 1 && digits[digitCount - 1] == '0' ) {
        digits[--digitCount] = '\0';
    }

    java::String out = negative ? "-" : "";
    char one[2] = {'\0', '\0'};
    double magnitude = std::fabs(value);

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
    out += digitCount > 1 ? java::String(digits + 1) : java::String("0");
    out += "E";
    out += java::String::valueOf(exponent);
    return out;
}

int Integer::parseInt(const java::String& text)
{
    const char* start = text.c_str();
    if ( *start == '\0' ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    const char* p = start;
    if ( *p == '+' || *p == '-' ) {
        p++;
    }
    if ( *p == '\0' ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    for ( const char* q = p; *q != '\0'; q++ ) {
        if ( *q < '0' || *q > '9' ) {
            throw NumberFormatException(
                java::String("For input string: \"") + text + "\"");
        }
    }
    errno = 0;
    long long value = std::strtoll(start, nullptr, 10);
    if ( errno != 0 || value < MIN_VALUE || value > MAX_VALUE ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    return (int)value;
}

long long Long::parseLong(const java::String& text)
{
    const char* start = text.c_str();
    const char* p = start;
    if ( *p == '+' || *p == '-' ) {
        p++;
    }
    if ( *p == '\0' ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    for ( const char* q = p; *q != '\0'; q++ ) {
        if ( *q < '0' || *q > '9' ) {
            throw NumberFormatException(
                java::String("For input string: \"") + text + "\"");
        }
    }
    errno = 0;
    long long value = std::strtoll(start, nullptr, 10);
    if ( errno != 0 ) {
        throw NumberFormatException(
            java::String("For input string: \"") + text + "\"");
    }
    return value;
}

}

int java::Double::compare(double d1, double d2)
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
    bool nan1 = std::isnan(d1);
    bool nan2 = std::isnan(d2);
    if ( nan1 || nan2 ) {
        return nan1 == nan2 ? 0 : (nan1 ? 1 : -1);
    }
    bool negative1 = std::signbit(d1);
    bool negative2 = std::signbit(d2);
    if ( negative1 == negative2 ) {
        return 0;
    }
    return negative1 ? -1 : 1;
}


bool java::Double::isNaN(double value)
{
    return value != value;
}

bool java::Double::isFinite(double value)
{
    return value >= -MAX_VALUE && value <= MAX_VALUE;
}
