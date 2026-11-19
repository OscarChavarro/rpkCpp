#ifndef __VSDK__
#define __VSDK__

#include <stdarg.h>
#include <stddef.h>

// Language support for the legacy compilers of the port (gcc 2.7.2 and
// Turbo C++ 3.0, see the turboc README): every file that uses these macros
// includes this header directly.

// Turbo C++ 3.0 has no native bool: the port uses BOOLEAN, TRUE and FALSE
// everywhere (TRUE and FALSE may come from X11 or libjpeg, with the same
// values)
#ifndef VSDK_CHECK_BOOLEAN
#define BOOLEAN int
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#else
// Development check with a modern compiler (-DVSDK_CHECK_BOOLEAN): an int
// BOOLEAN keeps any value stored in it (i.e. `flags & MASK`, a char or a
// pointer) where a bool kept 0 or 1, so BOOLEAN becomes a class that only
// takes bool values, and the compiler reports every other value
class VsdkCheckedBoolean {
  public:
    VsdkCheckedBoolean() : value(false) {}
    VsdkCheckedBoolean(bool v) : value(v) {}
    VsdkCheckedBoolean(const VsdkCheckedBoolean& other) : value(other.value) {}
    VsdkCheckedBoolean(const volatile VsdkCheckedBoolean& other) :
        value(other.value) {}
    VsdkCheckedBoolean& operator=(const VsdkCheckedBoolean& other) {
        value = other.value;
        return *this;
    }
    operator bool() const { return value; }
    operator bool() const volatile { return value; }
    VsdkCheckedBoolean& operator|=(bool v) { value = value || v; return *this; }
    VsdkCheckedBoolean& operator&=(bool v) { value = value && v; return *this; }
  private:
    VsdkCheckedBoolean(char);
    VsdkCheckedBoolean(unsigned char);
    VsdkCheckedBoolean(short);
    VsdkCheckedBoolean(unsigned short);
    VsdkCheckedBoolean(int);
    VsdkCheckedBoolean(unsigned int);
    VsdkCheckedBoolean(long);
    VsdkCheckedBoolean(unsigned long);
    VsdkCheckedBoolean(long long);
    VsdkCheckedBoolean(unsigned long long);
    VsdkCheckedBoolean(float);
    VsdkCheckedBoolean(double);
    VsdkCheckedBoolean(const void *);
    bool value;
};
#define BOOLEAN VsdkCheckedBoolean
#undef TRUE
#undef FALSE
#define TRUE VsdkCheckedBoolean(true)
#define FALSE VsdkCheckedBoolean(false)
#endif

#ifdef __TURBOC__
// Keywords of later C++ versions, without effect on the generated code
#define explicit
// Turbo C++ 3.0 takes `long long` as a 32 bit `long`, without the LL
// suffixes: functions overloaded for `long` and `long long` only declare
// the `long long` version with VSDK_HAS_LONG_LONG
#define VSDK_LL(value) value##L
#define VSDK_ULL(value) value##UL
// Turbo C++ 3.0 has no snprintf: these write without checking the size
int snprintf(char *buffer, size_t size, const char *format, ...);
int vsnprintf(char *buffer, size_t size, const char *format,
    va_list arguments);
#else
#define VSDK_HAS_LONG_LONG
#define VSDK_LL(value) value##LL
#define VSDK_ULL(value) value##ULL
#endif

#ifndef __GNUC__
#define __attribute__(attributes)
#endif

// Turbo C++ 3.0 has no exceptions (VSDK_WITHOUT_EXCEPTIONS, that may also be
// given to a modern compiler to check this configuration): a throw reports
// the exception and ends the program, as an uncaught exception does, a try
// block just runs, and the catch clauses are left out with
// `#ifndef VSDK_WITHOUT_EXCEPTIONS`
#ifdef __TURBOC__
#define VSDK_WITHOUT_EXCEPTIONS
#endif
#ifdef VSDK_WITHOUT_EXCEPTIONS
class Throwable;
class VsdkUncaughtThrow {
  public:
    void operator<<(const Throwable &exception) const;
};
extern VsdkUncaughtThrow vsdkUncaughtThrow;
#define try
#define throw vsdkUncaughtThrow <<
#endif

/**
@return the value of `T()` (zero for numbers and pointers), that Turbo C++
3.0 does not take for pointer types. The argument only selects `T` (legacy
compilers lack explicit template arguments for functions).
*/
template <class T> T
vsdkDefaultValue(T *)
{
    static T value;
    return value;
}

class String;

class VSDK {
public:
    static const double EPSILON;

    static String formatDouble(double a);
    static String formatDouble(double a, int digits);
};


#endif
