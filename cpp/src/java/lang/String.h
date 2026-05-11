#ifndef JAVA_STRING__
#define JAVA_STRING__

#include <cstdarg>

namespace java {

class String {
  private:
    char *value;

    void
    assignText(const char *text);

  public:
    String();
    String(const String &other);
    explicit String(const char *text);
    ~String();

    void
    dispose();

    String &
    operator=(const String &other);

    const char *
    toCString() const;

    int
    length() const;

    bool
    isEmpty() const;

    char
    charAt(int index) const;

    bool
    equals(const String &other) const;

    bool
    equals(const char *other) const;

    String
    substring(int beginIndex) const;

    String
    substring(int beginIndex, int endIndex) const;

    int
    indexOf(char token, int fromIndex = 0) const;

    bool
    startsWith(const char *prefix) const;

    static String
    formatCStringToJavaString(const char *format, va_list arguments);
};

}

#endif
