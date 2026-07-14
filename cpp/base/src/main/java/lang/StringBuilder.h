#ifndef JAVA_STRING_BUILDER__
#define JAVA_STRING_BUILDER__

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

    String
    toString() const;
};

}

#endif
