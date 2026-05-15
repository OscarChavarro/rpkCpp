#ifndef __JAVA_UTIL_FORMATTER__
#define __JAVA_UTIL_FORMATTER__

#include <stdarg.h>

#include "java/io/OutputStream.h"
#include "java/lang/String.h"

class Formatter {
  public:
    Formatter();
    explicit Formatter(OutputStream *outputStream);
    ~Formatter();

    OutputStream *
    out() const;

    void
    flush();

    void
    close();

    Formatter &
    format(const char *format, ...);

    static int
    format(char *buffer, int bufferSize, const char *format, ...);

    static int
    vformat(char *buffer, int bufferSize, const char *format, va_list arguments);

  private:
    static String
    appendText(const String &left, const String &right);

    static String
    formatToString(const char *formatText, va_list arguments);

    OutputStream *outputStream;
    String content;
    bool closed;
};


#endif
