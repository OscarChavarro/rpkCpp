#ifndef __STRING_BUILDER__
#define __STRING_BUILDER__

#include "java/lang/String.h"
namespace java {

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

    StringBuilder &
    append(long long value);

    StringBuilder &
    append(unsigned int value);

    StringBuilder &
    append(unsigned long value);

    StringBuilder &
    append(unsigned long long value);

    /// As Java, with the text of `Double.toString` (i.e. "1.0")
    StringBuilder &
    append(double value);

    /// As Java: "true" or "false"
    StringBuilder &
    append(bool value);

    String
    toString() const;
};

}

#endif
