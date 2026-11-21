#ifndef __DOUBLE__
#define __DOUBLE__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"


class Double {
  public:
static const double MIN_VALUE;
static const double MAX_VALUE;
/// Smallest positive normal value (2^-1022)
static const double MIN_NORMAL;
static const double POSITIVE_INFINITY;
static const double NEGATIVE_INFINITY;

// C++ port note: Turbo C++ 3.0 has no NaN literal; the constant is built at
// run time from its bytes (Cpp98Support::nan)
static const double NaN;
static BOOLEAN isNaN(double value);
static BOOLEAN isFinite(double value);

/**
Emulation of `Double.parseDouble`: leading and trailing white space is
ignored.
@param text text with a decimal number
@return the number
@throws NumberFormatException if the text is not a number
*/
static double parseDouble(const String& text);

/**
Emulation of `Double.toString`: the shortest decimal text that reads
back as the same number, with at least one decimal digit ("10.0"), in
computerized scientific notation ("1.0E7") for magnitudes out of
[1e-3, 1e7).
@param value number to write
@return the text of the number
*/
static String toString(double value);

/**
Emulation of `Double.compare`: a total order where -0.0 is smaller than
0.0, and NaN is equal to itself and greater than any other value.
@param d1 first value
@param d2 second value
@return negative, zero or positive as `d1` is less than, equal to or
greater than `d2`
*/
static int compare(double d1, double d2);
};


#endif
