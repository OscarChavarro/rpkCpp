#ifndef __STRING__
#define __STRING__

#include <cstdarg>
namespace java {

class String {
  private:
    char *value;

    void
    assignText(const char *text);

  public:
    static constexpr int npos = -1;

    String();
    String(const String &other);
    String(const char *text);
    ~String();

    void
    dispose();

    String &
    operator=(const String &other);

    String &
    operator=(const char *other);

    const char *
    toCString() const;

    const char *
    c_str() const;

    const char *
    data() const;

    int
    length() const;

    int
    size() const;

    bool
    isEmpty() const;

    bool
    empty() const;

    char
    charAt(int index) const;

    char
    operator[](int index) const;

    char &
    operator[](int index);

    char *
    begin();

    char *
    end();

    const char *
    begin() const;

    const char *
    end() const;

    bool
    equals(const String &other) const;

    bool
    equals(const char *other) const;

    String
    substring(int beginIndex) const;

    String
    substring(int beginIndex, int endIndex) const;

    String
    substr(int beginIndex) const;

    String
    substr(int beginIndex, int count) const;

    int
    indexOf(char token, int fromIndex = 0) const;

    int
    find(char token, int fromIndex = 0) const;

    int
    find_last_of(const char *tokens) const;

    String &
    erase(int index, int count = 1);

    bool
    startsWith(const char *prefix) const;

    /**
    Emulation of `String.startsWith(String, int)`.
    @return true if `prefix` occurs at `offset`
    */
    bool
    startsWith(const char *prefix, int offset) const;

    /**
    @return the index of the first occurrence of the text, or -1
    */
    int
    indexOf(const String &text, int fromIndex = 0) const;

    bool
    contains(const String &text) const;

    bool
    endsWith(const String &suffix) const;

    /**
    Emulation of `String.replace(CharSequence, CharSequence)`: replaces every
    occurrence of the target, from the start to the end.
    */
    String
    replace(const String &target, const String &replacement) const;

    /// ASCII only, as the texts of the port are
    String
    toUpperCase() const;

    /// ASCII only, as the texts of the port are
    String
    toLowerCase() const;

    bool
    equalsIgnoreCase(const String &other) const;

    /**
    @return the text without leading and trailing characters up to the
    space (as Java `trim`)
    */
    String
    trim() const;

    int
    rfind(char token) const;

    int
    rfind(const char *token) const;

    int
    rfind(const char *token, int fromIndex) const;

    int
    rfind(const String &token) const;

    String
    concat(const String &other) const;

    String
    concat(const char *other) const;

    String
    operator+(const String &other) const;

    String
    operator+(const char *other) const;

    String &
    operator+=(const String &other);

    String &
    operator+=(const char *other);

    String
    operator+(char other) const;

    String &
    operator+=(char other);

    bool
    operator==(const String &other) const;

    bool
    operator==(const char *other) const;

    bool
    operator!=(const String &other) const;

    bool
    operator!=(const char *other) const;

    bool
    operator<(const String &other) const;

    bool
    operator<=(const String &other) const;

    bool
    operator>(const String &other) const;

    bool
    operator>=(const String &other) const;

    static String
    valueOf(long long value);

    static String
    valueOf(unsigned long long value);

    static String
    valueOf(long value);

    static String
    valueOf(unsigned long value);

    static String
    valueOf(int value);

    static String
    valueOf(unsigned int value);

    static String
    valueOf(double value);

    static String
    valueOf(float value);

    static String
    formatCStringToJavaString(const char *format, va_list arguments);

    /**
    Emulation of `String.format`, with the conversions of C `printf`.
    */
    static String
    format(const char *format, ...);

};

String operator+(const char *left, const String &right);

}

#endif
