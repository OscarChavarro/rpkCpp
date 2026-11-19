#ifndef __STRING_BUILDER__
#define __STRING_BUILDER__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

class StringBuilder {
  private:
char *value;
int lengthValue;
int capacity;

void
ensureCapacity(int requiredLength);

void
assignFromCString(const char *text);

  public:
StringBuilder();
StringBuilder(const StringBuilder &other);
explicit StringBuilder(const String &text);
~StringBuilder();

void
dispose();

StringBuilder &
operator=(const StringBuilder &other);

int
length() const;

char
charAt(int index) const;

void
clear();

StringBuilder &
append(const String &text);

StringBuilder &
append(const char *text);

StringBuilder &
append(const char *text, int textLength);

StringBuilder &
append(char ch);

StringBuilder &
append(int value);

StringBuilder &
append(long value);

#ifdef VSDK_HAS_LONG_LONG
StringBuilder &
append(long long value);
#endif

/// As Java, with the text of `Double.toString` (i.e. "1.0")
StringBuilder &
append(double value);

/// As Java's append(boolean): "true" or "false" (BOOLEAN is an int, so
/// it cannot overload append)
StringBuilder &
appendBoolean(BOOLEAN value);

String
toString() const;
};


#endif
